/*=====================================================================*/
/*          DEPFET file converter (RAW->LCIO)                          */
/*          Author: Julia Furletova                                    */
/*                (julia@mail.desy.de or furletova@physik.uni-bonn.de) */
/*          Created   02 nov 2007                                      */
/*          Modified                                                   */
/*=====================================================================*/

// user includes
#include "DEPFET_FR.h"
#include "Mod_DEPF.h"
#include "DEPFETReader.h"
#include "EUTelRunHeaderImpl.h"
#include "EUTelEventImpl.h"


// marlin includes
#include "marlin/Processor.h"
#include "marlin/DataSourceProcessor.h"
#include "marlin/ProcessorMgr.h"

// lcio includes
#include <IMPL/LCEventImpl.h>
#include <IMPL/LCCollectionVec.h>
#include <IMPL/TrackerRawDataImpl.h>
#include <UTIL/CellIDEncoder.h>
#include <UTIL/LCTime.h>
// #include <UTIL/LCTOOLS.h>

// system includes
#include <fstream>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>

using namespace std;
using namespace marlin;
using namespace DEPFET;
using namespace eutelescope;
int  DECODINGDCDFPGA=1;

// Map of SWB channels (turned on consecutively) to  
// PXD5 double rows

int SWBChannelMap[16]   = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

int FPGAToDrainMap[128]={ 0, 4, 8, 12, 2, 6, 10, 14, 124, 120,
                        116, 112, 126, 122, 118, 114, 16, 20, 24, 28, 
                         18, 22, 26, 30, 108, 104, 100, 96, 110, 106,
                         102, 98, 36, 35, 37, 34, 52, 51, 53, 50,
                         68,67, 69, 66, 84, 83, 85, 82, 38, 33,
                         39,32, 54, 49, 55, 48, 70, 65, 71, 64,
                         86, 81, 87, 80,  1, 5, 9, 13, 3, 7, 
                         11, 15, 125, 121, 117, 113, 127, 123, 119, 115, 17, 
                         21, 25, 29, 19, 23, 27, 31, 109, 105,
                         101, 97, 111, 107, 103, 99, 44, 43, 45,
                         42, 60, 59, 61, 58, 76, 75, 77, 74, 92, 
                         91, 93, 90, 46, 41, 47, 40, 62, 57, 63,
                         56, 78, 73, 79, 72, 94, 89, 95, 88 };


/*=====================================================================*/
DEPFETReader::DEPFETReader ():DataSourceProcessor  ("DEPFETReader") {
/*=====================================================================*/
 #define SETUP_MAX 6

  _description =
    "Reads data files and creates LCEvent with TrackerRawData collection.\n"
    "Make sure to not specify any LCIOInputFiles in the steering in order to read DEPFET files.";


  IntVec excludePlanes;

  IntVec setupPlanes;
  for(int j=0;j<SETUP_MAX;j++){
      excludePlanes.push_back(0);
      setupPlanes.push_back(0);
  }


  registerProcessorParameter ("FileName", "Input file",
                              _fileName, std::string ("input.dat"));
  registerProcessorParameter ("runNumber", "RunNumber",
                              _runNumber, static_cast < int >(01));

  registerProcessorParameter ("NoOfXPixel", "Number of pixels along X",
                              _noOfXPixel, static_cast < int >(64));
  registerProcessorParameter ("NoOfYPixel", "Number of pixels along Y",
                              _noOfYPixel, static_cast < int >(128));
  registerProcessorParameter ("Fold", "Readout type (2-fold or 4 -fold)",
                              _Fold, static_cast < int >(2));
 registerProcessorParameter ("Excludeplane", "Exclude one or more planes: 9 14 1 4 (module ID)",
                              _excludePlane,excludePlanes);
  registerProcessorParameter ("SETUP", "Setup starting from beam 9 14 1 4 (module ID)",
                              _setupPlane,setupPlanes);
  registerProcessorParameter ("FILE_FLAG", "To Skip headers 9999 ",
                              _yearflag,static_cast < int >(07));

  registerProcessorParameter ("UseDCDBMapping", "Use internal maps for DCDB/SWB encoding ",
                              _useDCDBMapping ,static_cast < bool >(false));
 registerProcessorParameter ("SkipStartGate",
                              "Skip 4 rows after StartGate",
                              _SkipStartGate, static_cast < int >(0));


}

/*=====================================================================*/
 DEPFETReader * DEPFETReader::newProcessor () {
/*=====================================================================*/

   return new DEPFETReader;
}

/*=====================================================================*/
 void DEPFETReader::init () {
/*=====================================================================*/

    printParameters ();
}

/*=====================================================================*/
 void DEPFETReader::readDataSource (int Ntrig) {
/*=====================================================================*/
    EUTelEventImpl *event = NULL;
    LCCollectionVec *rawData = NULL;
    event_DEPFET  copy_evt[MAXMOD];
    int lenEVENT[MAXMOD];
    int startgate[MAXMOD];

    TrackerRawDataImpl *rawMatrix[20];
    int rc,eventNumber;
    static int First=1;
    char filename[80];
    int Nmodtot,iprint=0,i,idet;
    int Nex,Ndet,kk;
    int *DATA;
    signed short * DATA1; 
    Mod_DEPF *DEPFET[10];
    DEPFET_FR  *myfile;
    static int MOD2ID[32], ID2MOD[32];
    int DEPFET128=0;
    int DEV_TYPE[MAXMOD];

//     sprintf(filename,"input.dat");
    sprintf(filename,"%s",_fileName.c_str());
    myfile=new DEPFET_FR(filename,Ntrig,&rc);
    printf("RC=%d\n",rc);
    if (rc<0) {printf("problem with opening file\n"); exit (-1); };
   /*=====================================================================*/
   /*                     READ Group HEADER                               */
   /*=====================================================================*/
    //if(_yearflag==9999 ) {Nmodtot=6; goto year09a;}
    if(_yearflag==9999 ) {Nmodtot=1; goto year09a;}

     Nmodtot=myfile->READ_GROUP_HEADER(iprint);
     printf("Nmodtot =%d \n",Nmodtot);
     //   Nmodtot=1;
 //    int _runNumber=1802;
     if(_yearflag==06 ) Nmodtot=5;
     // if(_yearflag==07) Nmodtot=2;
     year09a:;
     Nex=0; Ndet=0;
     for(i=0;i<SETUP_MAX;i++) {
         if (_setupPlane[i]<=0) break;
         ID2MOD[_setupPlane[i]]=-1;
         printf("-> ALL::  i=%d  Mod=%d  iMOD=%d \n"
                ,i,_setupPlane[i],ID2MOD[i]);
         for(int ii=0;ii<SETUP_MAX;ii++) {
             if(_excludePlane[ii]==_setupPlane[i]) { Nex+=1; goto excl; }
         }
         ID2MOD[Ndet]=_setupPlane[i];
         MOD2ID[_setupPlane[i]]=Ndet;
         printf("   SET::  i=%d  Mod=%d MOD2ID=%d iMOD=%d \n"
                ,i,_setupPlane[i],MOD2ID[_setupPlane[i]], ID2MOD[Ndet]);

         DEPFET[Ndet] = new Mod_DEPF(ID2MOD[Ndet]);
         Ndet++;
     excl:;
     }
     printf (" DEPFET  Init:: Ndet=%d Nex=%d\n",Ndet, Nex);
     if(_yearflag==9999) goto year09b;
     kk=0;
     for( i=0;i<Nmodtot;i++) {
         myfile->READ_HEADER(iprint);
         printf("DEPFET_FR :: %d TYPE %d \n",  myfile->header.ModuleNo,myfile->header.DeviceType);
	 DEV_TYPE[i]=myfile->header.DeviceType;
         if (MOD2ID[myfile->header.ModuleNo]<0) continue;
         kk++;
         printf("MMMMM=%d \n",myfile->header.ModuleNo);
         printf(".i=%d, kk=%d, id2mod=%d, Modid=%d, mod2id=%d........\n"
                ,i,kk,ID2MOD[kk], myfile->header.ModuleNo,MOD2ID[myfile->header.ModuleNo]);
     }
 year09b:;
     printf ("================================== \n");
     printf ("Number of Modules %d , Run Number=%d\n",Nmodtot,_runNumber);
     printf ("================================== EOF HEADER\n");
     eventNumber=0;

   /*=====================================================================*/
   /*                 READ DATA                                           */
   /*=====================================================================*/

     while (true)  {  idet=0; kk=-1;

     if(iprint) printf ("================================== START READ DATA\n");

     rc=myfile->READ_EVENT_HEADER(iprint,eventNumber); //---read 2 words from event(Header+Trigger)

     /*-------------Nevents=Ntrig...go to end---------*/
     if(rc==-1) goto aaa;
     /*-------------Event Group HEADER----------------*/
     if(rc==100)  { /*---------BOR event---------*/
                    // --goto skip_1;

     if(!First) {
         event->addCollection (rawData, "rawdata");
         ProcessorMgr::instance ()->processEvent (static_cast<LCEventImpl*> (event));
         delete event;
     }
     /*---------BOR event---------*/
     else { First=0;
         printf("FIRST event:: start fill HEADER\n");
         // in the case it is the first run so we need to process and
         // write out the run header.

         auto_ptr<IMPL::LCRunHeaderImpl> lcHeader  ( new IMPL::LCRunHeaderImpl );
         auto_ptr<EUTelRunHeaderImpl>    runHeader ( new EUTelRunHeaderImpl (lcHeader.get()) );
         runHeader->addProcessor( type() );
         runHeader->lcRunHeader()->setDescription(" Events read from DEPFET input file: " + _fileName);
         runHeader->lcRunHeader()->setRunNumber (_runNumber);
         runHeader->setHeaderVersion (0.0001);
         runHeader->setDataType (EUTELESCOPE::CONVDATA);
         runHeader->setDateTime ();
         runHeader->addIntermediateFile (_fileName);
         runHeader->addProcessor (_processorName);
         // this is a mistake here only for testing....
         runHeader->setNoOfEvent(Ntrig);
         //////////////////////////////////////////////
         //////////////////////////////////////////////
         runHeader->setNoOfDetector(Ndet);
         runHeader->setMinX(IntVec(Ndet, 0));
         runHeader->setMaxX(IntVec(Ndet, _noOfXPixel - 1));
         runHeader->setMinY(IntVec(Ndet, 0));
         runHeader->setMaxY(IntVec(Ndet, _noOfYPixel - 1));

         runHeader->lcRunHeader()->setDetectorName("DEPFET");
         //UTIL::LCTOOLS::dumpRunHeader(runHeader);

         // process the run header
         ProcessorMgr::instance ()->processRunHeader ( static_cast<lcio::LCRunHeader*> ( lcHeader.release()) );

         // end of first event
         _isFirstEvent = false;
         printf("END BOR event:: \n");

     };    /*---------END BOR event---------*/

     event = new EUTelEventImpl;
     event->setDetectorName("DEPFET");
     event->setEventType(kDE);
     LCTime * now = new LCTime;
     event->setTimeStamp(now->timeStamp());
     delete now;
     rawData = new LCCollectionVec (LCIO::TRACKERRAWDATA); //<---- Only for RAW data

     eventNumber++;  printf("Event number=%d  Trigger=%d \n",eventNumber,myfile->header.Triggernumber);
     if(eventNumber%10==0) printf("Event number=%d  Trigger=%d \n",eventNumber,myfile->header.Triggernumber);
//     if(eventNumber%16==0) { printf("SKIP Event number=%d  Trigger=%d \n",eventNumber,myfile->header.Triggernumber); goto myskip;}
//       eventNumber=myfile->header.Triggernumber;
     if(iprint) printf("EventNumber=%d\n",eventNumber);
     event->setRunNumber (_runNumber);
     event->setEventNumber (eventNumber);


     } // skip_1:  //--  if rc!=100  --> end group header
     // sleep(10);
     kk=0;
     do { idet++;
         if(iprint) printf ("===============> idet= %d  rc=%d\n",idet,rc);
         rc=myfile->READ_EVENT_HEADER(iprint,eventNumber);
         if(iprint)printf("READ_DATA rc=%d \n",rc);
         /*-------------Module DATA HEADER----------------*/
         if(rc==1) goto aaa;
         if(rc==2) {   // goto skip_2;
 
	     if(myfile->header.DeviceType==0x3){ //S3B
	     if(_Fold==4){
		 _noOfXPixel=32;
		 _noOfYPixel=512;
		 DEPFET128=1;
	     } else {
		 _noOfXPixel=64;
		 _noOfYPixel=256;
		 DEPFET128=1;
	     }
	 }   else  if(myfile->header.DeviceType==0x4){ //DCD
	     if(_Fold==4){
		 DEPFET128=0; 
		 _noOfXPixel=32;
		 _noOfYPixel=64; 
	     }else {   
		 DEPFET128=0; 
		 _noOfXPixel=64;
		 _noOfYPixel=32;   
	     } 
	 }else {
             DEPFET128=0;
	     _noOfXPixel=64;
	     _noOfYPixel=128;    
	 };
         DATA= new int [_noOfXPixel*_noOfYPixel];
	 DATA1= new signed short [_noOfXPixel*_noOfYPixel];
	 if(myfile->header.DeviceType==0x4) { 
	     rc=myfile->READ_DEPFET_EVENT_DCD(iprint,_SkipStartGate,&copy_evt[idet-1],&lenEVENT[i]);
	     if(rc==1) goto aaa;
	 } else { rc=myfile->READ_DEPFET_EVENT1(iprint,_SkipStartGate,&copy_evt[idet-1],&lenEVENT[i]);
	     if(rc) goto aaa;
	 }
         int iMOD=MOD2ID[myfile->header.ModuleNo];
         if (iMOD<0) continue;
	 printf("iMod=%d,Module= %d \n",iMOD,myfile->header.ModuleNo);
	 //     if (iprint)
//	 printf("Start idet=%d, imod=%d %d,%d Trig=%d startgate =%d \n",idet-1, iMOD, DEPFET[iMOD]->ModID(),myfile->header.ModuleNo,myfile->header.Triggernumber,copy_evt[idet-1].Startgate);

         rawMatrix[iMOD] = new TrackerRawDataImpl;
         CellIDEncoder < TrackerRawDataImpl > idEncoder (EUTELESCOPE::MATRIXDEFAULTENCODING, rawData);
//        idEncoder["sensorID"] = DEPFET[iMOD]->ModID();
	 //        idEncoder["sensorID"] = myfile->header.ModuleNo;
         /*  0 1 2 3 4 5 ...*/
         idEncoder["sensorID"] = iMOD;
         idEncoder["xMin"] = 0;
         idEncoder["xMax"] = _noOfXPixel - 1;
         idEncoder["yMin"] = 0;
         idEncoder["yMax"] = _noOfYPixel - 1;
         idEncoder.setCellID (rawMatrix[iMOD]);

	 short *ADC; 
          ADC=(short *) copy_evt[idet-1].DATA;
	  int startgate=copy_evt[idet-1].Startgate;
        
	  //  for(int i=0;i<10;i++)  printf("startgate =%d ADC=%d  \n",startgate,ADC[i]&0xffff);
          if(myfile->header.DeviceType==0x3 ){

	      if(_Fold==2 && DEPFET128==1) {  /*-------------------- S3B system!!! -----------*/
		  for (int gate = 0; gate < 128; gate ++)  {      // Pixel sind uebereinander angeordnet
		      int readout_gate;
		      //	      if (Direction == false) // default
		      readout_gate = (startgate + gate)%128;
		      //else
		      //readout_gate = (startgate[ii] + 64 - gate)%64;
		      
		      int odderon;
		      for (int col = 0; col < 32; col += 2) {
			  odderon = readout_gate %2;      //  = 0 for even, = 1 for odd
			  
			  // eight cases:       -- new! Considers Matrix->Curo Bondpad mismatch! --
			  // 1. U, ramzelle 0 --> row 1, col 63
			  //              int dummy = RAM16[(frame*64*128) + (gate*128) + (col*4)];
			  //DepfetFrame[63-col][(readout_gate*2) +1 -odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4)];
			  DATA[(63-col)*_noOfYPixel+(readout_gate*2) +1 -odderon]= ADC[(gate*128) + (col*4)]&0xffff;
			  /*
			    printf("TDepfetProducerLab::BuildEvent(frame=%d):: RAM_A[%d]=%d  DepfetFrame[%d][%d]=%f\n",frame
			    ,(frame*64*128) + (gate*128) + (col*4),RAM_A[(frame*64*128) + (gate*128) + (col*4)]
			    ,63-col , (readout_gate*2) +1 -odderon  ,  DepfetFrame[63-col][(readout_gate*2) +1 -odderon]);
			  */
			  // 2. D, ramzelle 1 --> row 0, col 0
			  //              dummy = RAM16[(frame*64*128) + (gate*128) + (col*4) +1];
			  //DepfetFrame[col][(readout_gate*2) +odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +1];
			  DATA[col*_noOfYPixel+(readout_gate*2) +odderon]= ADC[(gate*128) + (col*4) +1]&0xffff;
			  // 3. U, ramzelle 2 --> row 1, col 62
			  //DepfetFrame[63 -1 - col][(readout_gate*2) +1 -odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +2];
			  DATA[(63-1-col)*_noOfYPixel+(readout_gate*2) +1 -odderon]= ADC[(gate*128) + (col*4) +2]&0xffff;
			  // 4. D, ramzelle 3 --> row 0, col 1
			  //DepfetFrame[col +1][(readout_gate*2) +odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +3];
			  DATA[(col+1)*_noOfYPixel+(readout_gate*2) +odderon]= ADC[(gate*128) + (col*4) +3]&0xffff;
			  // 5. U, ramzelle 4 --> row 0, col 63
			  //DepfetFrame[63 - col][(readout_gate*2) +odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +4];
			  DATA[(63-col)*_noOfYPixel+(readout_gate*2) +odderon]= ADC[(gate*128) + (col*4) +4]&0xffff;
			  // 6. D, ramzelle 5 --> row 1, col 0
			  //DepfetFrame[col][(readout_gate*2) +1 -odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +5];
			  DATA[col*_noOfYPixel+(readout_gate*2) +1 -odderon]= ADC[(gate*128) + (col*4) +5]&0xffff;
			  // 7. U, ramzelle 6 --> row 0, col 62
			  //DepfetFrame[63 -1 - col][(readout_gate*2) +odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +6];
			  DATA[(63-1-col)*_noOfYPixel+(readout_gate*2) +odderon]= ADC[(gate*128) + (col*4) +6]&0xffff;
			  // 8. D, ramzelle 7 --> row 1, col 1
			  //DepfetFrame[col +1][(readout_gate*2) +1 -odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +7];
			  DATA[(col+1)*_noOfYPixel+(readout_gate*2) +1 -odderon]= ADC[(gate*128) + (col*4) +7]&0xffff;
		      } // col
		  } // gates
	      } else  if (_Fold==4) { // S3B 4-fold readout
 
		  for (int gate = 0; gate < 128; gate ++)  {      // Pixel sind uebereinander angeordnet
		      int readout_gate;
		      readout_gate = (startgate + gate)%128;
		      
		      for (int col = 0; col < 16; col += 1) {
			  
			  DATA[ (31-col)  *_noOfYPixel   + (readout_gate*4) +3 ]  = ADC[(gate*128) + (col*8) +0 ]&0xffff;
			  
			  DATA[  col      *_noOfYPixel   + (readout_gate*4) +0 ]  = ADC[(gate*128) + (col*8) +1 ]&0xffff;
			  
			  DATA[ (31-col)  *_noOfYPixel   + (readout_gate*4) +2 ]  = ADC[(gate*128) + (col*8) +2 ]&0xffff;
			  
			  DATA[  col      *_noOfYPixel   + (readout_gate*4) +1 ]  = ADC[(gate*128) + (col*8) +3 ]&0xffff;
			  
			  DATA[ (31-col)  *_noOfYPixel   + (readout_gate*4) +1 ]  = ADC[(gate*128) + (col*8) +4 ]&0xffff;
			  
			  DATA[  col      *_noOfYPixel  + (readout_gate*4) +2 ]  = ADC[(gate*128) + (col*8) +5 ]&0xffff;
			  
			  DATA[ (31-col)  *_noOfYPixel   + (readout_gate*4) +0 ]  = ADC[(gate*128) + (col*8) +6 ]&0xffff;
			  
			  DATA[  col      *_noOfYPixel   + (readout_gate*4) +3 ]  = ADC[(gate*128) + (col*8) +7 ]&0xffff;

			  //		  printf("readout_gate=%d NUM_COL=%d  NUM_ROW=%d data=%d \n",readout_gate,_noOfXPixel,_noOfYPixel, DATA[  col      *_noOfYPixel   + (readout_gate*4) +3 ]);
			  
		      } // col
		  } // gates
		  

	      }
	      

	  } else if(myfile->header.DeviceType==0x4) { /*----------- DCD system!!! --------*/
	      signed char *v4data=(signed char *)copy_evt[idet-1].DATA;
              if(_Fold==2 ){
		  if(_useDCDBMapping){
		      // printf("=> try with internal maps \n");
		      // do not touch maps above!! and cross fingers 
		      
		      int iData = -1;  // pointer to raw data 
		      int noOfDCDBChannels = _noOfXPixel*2; // used channels only 
		      int noOfSWBChannels = _noOfYPixel/2;  // used channels only 
		      
		      for (int offset = 0; offset < noOfSWBChannels; offset ++)  { // loop over SWB channels
			  // this is the SWB channel/pad switched on
			  int iSWB = (startgate + offset)%( noOfSWBChannels );
			  // which is bonded to PXD5 double row number  
			  int iDoubleRow = SWBChannelMap[iSWB];  
			  
			  //printf("=> iSWB=%d \n",iSWB);
			  
			  for (int iFPGA = 0; iFPGA < noOfDCDBChannels; iFPGA ++) { // loop over DCDB channels 
			      iData++;      		  
			      
			      // position iFPGA in serial output is mapped to drain line number 
			      int iDrain = FPGAToDrainMap[iFPGA];
			      
			      // irow and icol are referring to physical pixels on sensor
			      int icol, irow;
			      icol = iDrain/2; 
			      
			      // double rows are counted 0,1,2,... starting from DCDB and first 
			      // connected double row is even!!
			      
			      if (iDoubleRow%2 == 0) {
				  // ok, this double row is even 
				  if ( iDrain%2 == 0) irow = 2*iDoubleRow;
				  else irow = 2*iDoubleRow+1;
			      }  
			      else  {
				  // ok, this double row is odd
				  if ( iDrain%2 == 0) irow = 2*iDoubleRow+1 ;
				  else irow = 2*iDoubleRow;
			      } 
			      
			      DATA1[ icol*_noOfYPixel + irow ]=(signed short) v4data[iData]; 
			      
			      //printf("=> FILL iData=%d irow=%d icol=%d  data=%d \n",iData, irow ,icol , DATA1[icol*_noOfYPixel + irow ]);
			  }
		      }
		      
		  } else {  
                
		      // all encodings done on daq (only bonn laser data)
		      
		      
		      int ipix=-1; 
		      for (int offset = 0; offset < _noOfYPixel; offset++)  { // loop over Switcher channels
			  
			  int igate = (startgate + offset)%(_noOfYPixel);
			  
			  for (int idrain = 0; idrain < _noOfXPixel; idrain++) { // loop over DCD channels 
			      ipix++;                       
			      
			      DATA1[ idrain*_noOfYPixel + igate ]=(signed short) v4data[ipix];        
			      //printf("=> FILL ipix=%d data=%d \n",ipix, DATA1[ idrain*_noOfYPixel + igate  ]);
			      
			      
			  }     
		      }
		      
		  }
		  
		  
		  
	      } else if(_Fold==4){
		  
		  int ipix=-1;
//		  signed char *v4data=(signed char *)copy_evt[idet-1].DATA;
		  
		  int icold,icol,irow;
		  
		  for (int gate = 0; gate < _noOfYPixel/4; gate ++)   { // loop over Switcher channels
			  
			  int irowd1 = (startgate + gate)%(_noOfYPixel/4);
			  //     irowd=SWITCHERBMAP[irowd1];
			  for (int icoldcd = 0; icoldcd < _noOfXPixel*4 ; icoldcd ++) { // loop over DCD channels
				  if(_useDCDBMapping) { icold=FPGAToDrainMap[icoldcd];}
				  else {icold=icoldcd;}
				  ipix++; 
				  
				  icol=(icold/4)%(_noOfXPixel);
				  irow=(irowd1*4+icold%4 )%(_noOfYPixel);
				  
				  DATA1[(icol)*_noOfYPixel+irow]= (signed short) v4data[ipix]; 
				  //      DEPFET[i]->XYHITS->Fill(icol,irow,data);
				  //			  printf("DATA=%d icol=%d, irow=%d\n",DATA1[(icol)*_noOfYPixel+irow],icol,irow);
			      }
			  }
		      
		      
		  }
		  
			   
	  } else {/*-------------------- S3A system!!! ------------*/
	      for (int ipix=0;ipix<_noOfYPixel*_noOfXPixel;ipix++) { //-- raspakowka daty ---- loop 8000
		      int x=copy_evt[idet-1].DATA[ipix]>>16&0x3F;
		      int y=copy_evt[idet-1].DATA[ipix]>>22&0x7F;
		      DATA[x*_noOfYPixel+y]= copy_evt[idet-1].DATA[ipix]&0xffff;//-- replace with memcpy ???
		      
		  }
	 }
          int ipix=0;
         for (int yPixel = 0; yPixel < _noOfYPixel; yPixel++) {
             for (int xPixel = 0; xPixel < _noOfXPixel; xPixel++) {
		 //    printf("xPixel =%d yPixel=%d DATA=%d \n",xPixel, yPixel, DATA[xPixel*_noOfYPixel + yPixel] ); 
		 // rawMatrix[iMOD]->adcValues ().push_back (DATA[xPixel*_noOfYPixel + yPixel]);

                 if(myfile->header.DeviceType==0x4) {rawMatrix[iMOD]->adcValues ().push_back (DATA1[xPixel*_noOfYPixel + yPixel]);

                 } else { rawMatrix[iMOD]->adcValues ().push_back (DATA[xPixel*_noOfYPixel + yPixel]);}


                 if(iprint==6 && ipix%8000)printf("Det=%d ADCvalue=%d,  ipix=%d %d,\n",iMOD,DATA[xPixel*_noOfYPixel +  yPixel],xPixel,yPixel);
                 ipix++;
             }
         }
//       rawData->push_back (rawMatrix[iMOD]);
	 delete DATA; delete DATA1; 

         }  // skip_2:  //end eventheader rc==2
     myskip:;
     } while (idet!=Nmodtot);
     printf("-------------------------------\n");
     for (int ii=0;ii<Ndet;ii++)  rawData->push_back (rawMatrix[ii]);

     }; // end while(true)
 aaa:
     event = new EUTelEventImpl;
     event->setDetectorName("DEPFET");
     LCTime * now = new LCTime;
     event->setTimeStamp(now->timeStamp());
     delete now;
     event->setRunNumber (_runNumber);
     event->setEventNumber (eventNumber++);
     event->setEventType(kEORE);
     ProcessorMgr::instance ()->processEvent (static_cast<LCEventImpl*> (event));
     delete event;
 }


/*=====================================================================*/

 void DEPFETReader::end () {
   message<MESSAGE> ("Successfully finished") ;
 }

/*=====================================================================*/


