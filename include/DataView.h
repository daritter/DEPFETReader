#ifndef DEPFET_DATAVIEW_H
#define DEPFET_DATAVIEW_H

#include <DEPFETReader/Exception.h>

namespace DEPFET {

  /** Class to provide and indexed view to a given data array. This class is
   * used to provide a readonly view of a chunk of data, probably as a
   * different type. This can be used to e.g. treat the memory of an array of
   * integers as a matrix of doubles or unsigned short.
   *
   * @tparam VIEWTYPE type of the view
   * @tparam STORAGETYPE type of the array
   */
  template < class VIEWTYPE, class STORAGETYPE = unsigned int > class DataView {
  public:
    /** Create a view of a given array.
     * The parameters nX and nY specify the dimensions of the view. If one is
     * zero, the other will be calculated from the size of the array. If both
     * are zero nY will be assumed to be 1
     *
     * @param data pointer to the array to use
     * @param size size of the array
     * @param nX size of the resulting view along X
     * @param nY size of the resulting view along Y
     */
    DataView(const STORAGETYPE* data, size_t size, size_t nX, size_t nY) {
      if (nX == 0 && nY == 0) { nY = 1; }
      if (nY == 0 && nX > 0) {
        nY = size * sizeof(STORAGETYPE) / (nX * sizeof(VIEWTYPE));
      } else if (nX == 0 && nY > 0) {
        nX = size * sizeof(STORAGETYPE) / (nY * sizeof(VIEWTYPE));
      } else if (nX * nY * sizeof(VIEWTYPE) > size * sizeof(STORAGETYPE)) {
        throw Exception("Data buffer not large enough");
      }
      m_data = (const VIEWTYPE*) data;
      m_nX = nX;
      m_nY = nY;
    }
    /** Return an element of the view
     * @param x X coordinate of the element
     * @param y Y coordinate of the element
     */
    const VIEWTYPE& operator()(size_t x, size_t y) const { return m_data[x * m_nY + y]; }
    /** Return an element from the flat view
     * @param index index of the element
     */
    const VIEWTYPE& operator[](size_t index) const { return m_data[index]; }
    /** Return the size of the view along X */
    size_t getSizeX() const { return m_nX; }
    /** Return the size of the view along Y */
    size_t getSizeY() const { return m_nY; }
  protected:
    /** Pointer to the data */
    const VIEWTYPE* m_data;
    /** Size of the view along X */
    size_t m_nX;
    /** Size of the view along Y */
    size_t m_nY;
  };
}
#endif
