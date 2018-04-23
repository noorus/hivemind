/**
* \mainpage
* blob - 8-neighbour connected components labelling and contours extractor
* =========================================================================
* v0.0.1
* Licensed under the MIT License
* (c) 2016 Vincent Cruz
*
* About:
* ------
* This header file provides a function to label and extract the contours
* of connected components (8-connected) from a region of an image.
* The implementation is based upon the following paper:
*
* "A linear-time component-labeling algorithm using contour tracing technique"
*  by Fu Chang, Chun-Jen Chen, and Chi-Jen Lu.
*
* Building:
* ---------
* Before include this file, add the following line
```
*     #define BLOB_IMPLEMENTATION
```
* in the file where you want to have the implementation.
*
* You can define `BLOB_MALLOC`, `BLOB_REALLOC` and `BLOB_FREE` before the
* include to replace `malloc`, `realloc` and `free`.
*
* You can define `BLOB_MEMSET` to replace memset.
*
* Errors messages (out of memory, invalid arguments) are displayed via
* `BLOB_ERROR`. By default this macro uses fprintf (hence adding a
* dependency to stdio.h). `BLOB_ERROR` can be defined to replace the
* default behaviour.
*
* Usage:
* ------
```
* int find_blobs( int16_t roi_x, int16_t roi_y, int16_t roi_w, int16_t roi_h,
*                 uint8_t *in, int16_t in_w, int16_t in_h,
*                 label_t **label, int16_t *label_w, int16_t *label_h,
*                 blob_t** blobs, int *count, int extract_internal );
```
*
* The first 4 parameters `roi_x`, `roi_y`, `roi_w`, `roi_h` defines the
* area in the input image where the labels and contours will be extracted.
* This area may be clamped to the input image dimension.
*
* `in`, `in_w`, `in_h` are the pointer to the input image and its dimensions.
* Note that the input image must be a 8bpp grayscale image. All pixels
* with a value of 0 are considered to be part of the background. All
* other values are treated as being part of the background.
*
* `label`, `label_w`, `label_h` are the pointer to the label buffer and its
* dimensions. The dimensions of the label buffer may be equal to `roi_w`
* and `roi_h`. A pixel in the label buffer may be equal to the label of
* the blob it belongs to, 0 for background and -1 for background pixels
* near blob countours.
*
* `blobs` and `count` are respectively the array and the number of
* extracted blobs. The blobs array must be freed using `destroy_blobs`
* function.
* A blob contains the label that was assigned to it, an array containing
* the coordinates of the pixels making its external contour, the number
* of internal contours (also called euler number), and the array of its
* internal contours. Note that the internal contour array is not built
* if `extract_internal` is set to 0.
*
* If `extract_internal` is set to 1, each blob will stores their internal
* contour points (if any). Otherwise, only the number of internal
* contours will be set.
*
* `find_blobs` returns 1 upon success or 0 if an error occured.
*
* Note:
* -----
* The memory management is far from being optimal.
* This piece of code is not meant to be "production ready".
*
*/

#include <cstdint>

namespace hivemind {

  namespace blob_algo {

    /**
    * Type for label buffer.
    * @warning The type must be signed.
    */
    typedef int16_t label_t;

    /**
    * Contour.
    * Array of contour pixel coordinates. A coordinate is stored as 2 int16_t.
    * The 1st one being X and the 2nd Y.
    */
    typedef struct
    {
      /** Number of points. **/
      int count;
      /** Number of allocated points. **/
      int capacity;
      /** Point array. **/
      int16_t *points;
    } contour_t;

    /**
    * Blob.
    */
    typedef struct
    {
      /** Label assigned to this blob. **/
      label_t label;
      /** External contour. **/
      contour_t  external;
      /** Internal contours (holes). **/
      contour_t *internal;
      /** Number of internal contour (also called euler number). **/
      int internal_count;
    } blob_t;

    /**
    * Compute connected components labels and contours.
    * @param [in]  roi_x   X coordinate of the upper left corner of the ROI.
    * @param [in]  roi_y   Y coordinate of the upper left corner of the ROI
    * @param [in]  roi_w   Width of the ROI.
    * @param [in]  roi_h   Height of the ROI.
    * @param [in]  in      Pointer to the input image buffer.
    * @param [in]  in_w    Width of the input image.
    * @param [in]  in_h    Height of the input image.
    * @param [out] label   Pointer to the label buffer.
    * @param [out] label_w Width of the label buffer (ROI clamped to image dimensions).
    * @param [out] label_h Height of the label buffer (ROI clamped to image dimensions).
    * @param [out] blobs   Array of extracted blobs.
    * @param [out] count   Number of extracted blobs.
    * @param [in]  extract_internal  Store internal contours in blob if set to 1.
    * @return 1 upon success or 0 if an error occured.
    */
    int find_blobs( int16_t roi_x, int16_t roi_y, int16_t roi_w, int16_t roi_h,
      uint8_t *in, int16_t in_w, int16_t in_h,
      label_t **label, int16_t *label_w, int16_t *label_h,
      blob_t** blobs, int *count, int extract_internal );

    /**
    * Destroy all blobs created by find_blobs.
    * @param [in] blobs Pointer to the array of blobs.
    * @param [in] count Number of blobs.
    */
    void destroy_blobs( blob_t *blobs, int count );

  }

}