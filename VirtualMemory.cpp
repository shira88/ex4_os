#include "VirtualMemory.h"
#include "PhysicalMemory.h"

//TODO DO THEY HAVE TO BE THE SAME
#define PAGE_TABLE_WIDTH OFFSET_WIDTH
#define PAGE_TABLE_BITS (1 << PAGE_TABLE_WIDTH) - 1;

uint64_t CalcPhysicalAddress (uint64_t frameAddress, uint64_t lineIndex)
{
  return (frameAddress * PAGE_SIZE) + lineIndex;
}

void InitializeFrame (uint64_t frameNum)
{
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    //TODO Maybe don't initialize it to 0
    uint64_t physicalAddress = CalcPhysicalAddress (frameNum, i);
    PMwrite (physicalAddress, 0);
  }
}

/*
 * Initialize the virtual memory.
 */
void VMinitialize ()
{
  InitializeFrame (0);
}

//TODO CHECK BITSHIFT HELPER FUNCTIONS
uint64_t CalculateLineIndex (uint64_t pageNum)
{
  return (pageNum >> ((TABLES_DEPTH - 1) * PAGE_TABLE_WIDTH)) & ((1 << PAGE_TABLE_WIDTH) -
  1);
}

uint64_t UpdatePageNum (uint64_t pageNum)
{
  return pageNum << PAGE_TABLE_WIDTH;
}

uint64_t RemoveOffset (uint64_t pageNum)
{
  return pageNum >> OFFSET_WIDTH;
}

uint64_t GetOffset (uint64_t pageNum)
{
  return pageNum & ((1 << OFFSET_WIDTH) - 1);
}

void MaxDFS (uint64_t frameNum, uint64_t *max, int iter)
{
  if (iter == TABLES_DEPTH)
  { return; }

  word_t tmpValue;

  for (uint64_t j = 0; j < PAGE_SIZE; j++)
  {

    PMread (CalcPhysicalAddress (frameNum, j), &tmpValue);

    if (tmpValue != 0)
    {

      if ((uint64_t) tmpValue > *max)
      {
        *max = tmpValue;
      }

      MaxDFS (tmpValue, max, iter + 1);
    }
  }
}

uint64_t ZeroTableDFS (uint64_t frameNum, uint64_t prevFrame, uint64_t
indexInPrevFrame, int iter, uint64_t dontTake)
{
  if (iter == TABLES_DEPTH)
  { return 0; }

  word_t tmpValue;
  uint64_t tmpReturn;
  bool allZero = true;

  for (uint64_t j = 0; j < PAGE_SIZE; j++)
  {
    PMread (CalcPhysicalAddress (frameNum, j), &tmpValue);

    if (tmpValue != 0)
    {
      allZero = false;
      tmpReturn = ZeroTableDFS (tmpValue, frameNum, j, iter + 1, dontTake);

      if (tmpReturn != 0)
      {
        return tmpReturn;
      }
    }
  }

  if (allZero && (prevFrame != -1) && (frameNum != dontTake))
  {
    PMwrite (CalcPhysicalAddress (prevFrame, indexInPrevFrame), 0);
    return frameNum;
  }

  return 0;

}

uint64_t GetMinValue (uint64_t a, uint64_t b)
{
  if (a < b)
  {
    return a;
  }
  return b;
}

uint64_t GetAbsulutValue (uint64_t num)
{
  if (num >= 0)
  {
    return num;
  }
  return -1 * num;
}

void SwapDFS (uint64_t pageSwappedIn, uint64_t pageSwappedOut,
              uint64_t *frameSwappedOut, uint64_t frameNum, uint64_t *max,
              int iter, uint64_t parentFrame, uint64_t *parentFrameFinal,
              uint64_t *evictedPageIndex)
{
  if (iter == TABLES_DEPTH)
  {
    // we reached a frame that contains a page (and not a page table)
    uint64_t cyclicalDistance =
        GetMinValue (NUM_PAGES - GetAbsulutValue (pageSwappedIn - pageSwappedOut),
                     GetAbsulutValue (pageSwappedIn - pageSwappedOut));

    if (cyclicalDistance > *max)
    {
      *max = cyclicalDistance;
      *frameSwappedOut = frameNum;
      *parentFrameFinal = parentFrame;
      *evictedPageIndex = pageSwappedOut;
    }

    return;
  }

  word_t tmpValue;
  pageSwappedOut = (pageSwappedOut << PAGE_TABLE_WIDTH);

  for (uint64_t j = 0; j < PAGE_SIZE; j++)
  {
    parentFrame = frameNum;
    PMread (frameNum * PAGE_SIZE + j, &tmpValue);

    if (tmpValue != 0)
    {
      SwapDFS (pageSwappedIn, pageSwappedOut + j, frameSwappedOut, tmpValue, max,
               iter + 1, parentFrame, parentFrameFinal, evictedPageIndex);
    }
  }
}

uint64_t getFreeFrame (uint64_t pageNum, uint64_t dontTake)
{
  uint64_t frameNumber = ZeroTableDFS (0, -1, -1, 0, dontTake);
  if (frameNumber != 0)
  {
    return frameNumber;
  }

  frameNumber = 0;
  MaxDFS (0, &frameNumber, 0);
  if (frameNumber + 1 < NUM_FRAMES)
  {
    return frameNumber + 1;
  }

  frameNumber = 0;
  uint64_t parentFrame = 0;
  uint64_t max = 0;
  uint64_t evictedPageIndex = 0;
  SwapDFS (pageNum,
           0,
           &frameNumber,
           0,
           &max,
           0,
           0,
           &parentFrame,
           &evictedPageIndex);
  word_t tmp;
  for (int i = 0; i < PAGE_SIZE; i++)
  {
    uint64_t physicalAddress = CalcPhysicalAddress (parentFrame, i);
    PMread (physicalAddress, &tmp);
    if (tmp == frameNumber)
    {
      PMwrite (physicalAddress, 0);
      break;
    }
  }

  PMevict (frameNumber, evictedPageIndex);
  return frameNumber;
}

uint64_t GetTargetFrame (uint64_t pageNum)
{
  /*
   * 1. for TABLE_DEPTH iterations:
   *    a.  the next page table we want to reach is located in the left
   *        PAGE_TABLE_WIDTH bits of the current virtual address.
   *        We need to extract this index.
   *    b.  Go to this index in the frame that matches the current page
   *        table and read the value.
   *    c.  if the value is 0:
   *            i.  it means the next page table doesn't exist yet, we need
   *                to create it (separate function).
   *                This function should find an empty frame (or evict one
   *                if all frames are full), initialize it to 0 and return
   *                the frame number.
   *            ii. write the returned frame number to the next page's line
   *                in the frame that matches the current page table.
   *        otherwise:
   *            i.  the next page table is stored in the frame with the
   *                index that matches the read value.
   * 2. When done, return the last frame index that was calculated.
   */

  // TODO: check validity of the virtual address somehow and return -1 if
  //  invalid
  uint64_t frameNum = 0;
  word_t tmpValue;
  uint64_t prevFrameNum = -1;
  uint64_t pageNumBitShift = pageNum;

  for (uint64_t i = 0; i < TABLES_DEPTH; i++)
  {
    uint64_t lineIndex = CalculateLineIndex (pageNumBitShift);
    uint64_t physicalAddress = CalcPhysicalAddress (frameNum, lineIndex);
    PMread (physicalAddress, &tmpValue);
    frameNum = tmpValue;

    if (frameNum == 0)
    {
      frameNum = getFreeFrame (pageNum, prevFrameNum);
      InitializeFrame (frameNum);
      PMwrite (physicalAddress, (word_t) frameNum);
    }

    prevFrameNum = frameNum;

      pageNumBitShift = UpdatePageNum (pageNumBitShift);
  }

  return frameNum;
}

uint64_t PrepareForReadWrite (uint64_t virtualAddress)
{
  /*
 * 1. remove the offset from virtualAddress to get the target page number.
 * 2. get the offset from virtualAddress to get the target word index of
 *    the target page.
 * 3. find the target page by traversing the tree and creating tables if
 *    needed.
 *    This should be a separate function that returns the empty frame number
 *    in the physical memory where we will insert the page's data.
 * 4. call PMRestore on the frame number from (3) and page number from (1)
 * 5. create the target physical address of the word in the frame by
 *    adding the offset from (2) to the frame number from (3).
 */
  uint64_t pageNumber = RemoveOffset (virtualAddress);
  uint64_t wordIndex = GetOffset (virtualAddress);
  uint64_t targetFrame = GetTargetFrame (pageNumber);

  PMrestore (targetFrame, pageNumber);

  uint64_t targetPhysicalAddress = CalcPhysicalAddress (targetFrame, wordIndex);
  return targetPhysicalAddress;
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread (uint64_t virtualAddress, word_t *value)
{
  uint64_t targetPhysicalAddress = PrepareForReadWrite (virtualAddress);

  if (targetPhysicalAddress == -1)
  {
    return 0;
  }

  PMread (targetPhysicalAddress, value);

  return 1;
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite (uint64_t virtualAddress, word_t value)
{
  uint64_t targetPhysicalAddress = PrepareForReadWrite (virtualAddress);

  if (targetPhysicalAddress == -1)
  {
    return 0;
  }

  PMwrite (targetPhysicalAddress, value);

  return 1;
}
