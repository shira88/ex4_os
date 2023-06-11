#include "VirtualMemory.h"
#include "PhysicalMemory.h"

//TODO DO THEY HAVE TO BE THE SAME
#define PAGE_TABLE_WIDTH OFFSET_WIDTH
#define PAGE_TABLE_BITS (1 << PAGE_TABLE_WIDTH) - 1;

/*H
 * Initialize the virtual memory.
 */
void VMinitialize ()
{

  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    //TODO Maybe don't initialize it to 0
    PMwrite (i, 0);
  }
}

//TODO CHECK BITSHIFT HELPER FUNCTIONS
uint64_t CalculateLineIndex (uint64_t virtualAddress)
{
  return (virtualAddress >> (VIRTUAL_ADDRESS_WIDTH - PAGE_TABLE_WIDTH))
         & ((1 << PAGE_TABLE_WIDTH) - 1);
}

uint64_t UpdateVirtualAddress (uint64_t virtualAddress)
{
  return virtualAddress << PAGE_TABLE_WIDTH;
}

void MaxBFS (uint64_t frameNum, uint64_t *max, int iter)
{

  if(iter == TABLES_DEPTH) { return; }

  word_t tmpValue;

  for (uint64_t j = 0; j < PAGE_SIZE; j++)
  {
    PMread (frameNum * PAGE_SIZE + j, &tmpValue);

    if (tmpValue != 0)
    {

      if((uint64_t)tmpValue > *max) {
        *max = tmpValue;
      }

      MaxBFS (tmpValue, max, iter+1);
    }
  }

}

uint64_t GetTargetPhysicalAddress (uint64_t virtualAddress)
{
  // TODO: check validity of the virtual address somehow and return -1 if
  //  invalid
  uint64_t frameAddress = 0;
  word_t tmpValue;

  for (uint64_t i = 0; i <= TABLES_DEPTH; i++)
  {
    uint64_t lineIndex = CalculateLineIndex (virtualAddress);
    uint64_t physicalAddress = frameAddress + lineIndex;
    PMread (physicalAddress, &tmpValue);
    frameAddress = tmpValue * PAGE_SIZE;

    if ((i != TABLES_DEPTH) && (frameAddress == 0))
    {
      //TODO DEAL WITH CASE THAT FRAME ADDRESS IS 0
    }

    virtualAddress = UpdateVirtualAddress (virtualAddress);
  }

  return frameAddress;
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
  uint64_t physicalAddress = GetTargetPhysicalAddress (virtualAddress);
  if (physicalAddress == -1)
  {
    return 0;
  }

  PMread (physicalAddress, value);
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
  uint64_t physicalAddress = GetTargetPhysicalAddress (virtualAddress);
  if (physicalAddress == -1)
  {
    return 0;
  }

  uint64_t frameIndex = physicalAddress / PAGE_SIZE;
  uint64_t pageIndex = CalcultePageIndex(virtualAddress); // remove the offset
  PMrestore (frameIndex, pageIndex);
  PMwrite (physicalAddress, value);
  return 1;
}
