// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, int vicfpn , int swpfpn)
{
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  //struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  struct vm_rg_struct *newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid); // Retrieve the current VMA

  if (cur_vma == NULL)
    return NULL; // Invalid VMA
  newrg = malloc(sizeof(struct vm_rg_struct));
  if (newrg == NULL)
    return NULL;
  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */
 newrg->rg_start = cur_vma->sbrk; // Start at the current break pointer
 newrg->rg_end = cur_vma->sbrk + alignedsz; // Extend by the aligned size

 /* Update the VMA's break pointer */
 cur_vma->sbrk += alignedsz;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  //struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  struct vm_area_struct *vma = caller->mm->mmap;

  while (vma != NULL) {
    // Check for overlap
    if (!(vmaend <= vma->vm_start || vmastart >= vma->vm_end)) {
      return -1; // Overlap detected
    }
    vma = vma->vm_next;
  }

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage =  inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (cur_vma == NULL || area == NULL)
    return -1;

  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
    return -1; /*Overlap and failed allocation */

  /* TODO: Obtain the new vm area based on vmaid */
  //cur_vma->vm_end... 
  // inc_limit_ret...
  cur_vma->vm_end = area->rg_end; // Extend the VMA's end boundary
  //int inc_limit_ret = cur_vma->vm_end - old_end; // Calculate the increment in the limit

  if (vm_map_ram(caller, area->rg_start, area->rg_end, 
                    old_end, incnumpage , newrg) < 0)
    return -1; /* Map the memory to MEMRAM */

  return 0;
}

// int read_mem(struct pcb_t *caller, addr_t source, BYTE *data)
// {
//   int pgn = PAGING_PGN(source);
//   int offset = PAGING_OFF(source);
//   uint32_t pte = caller->mm->pgd[pgn];

//   if (!PAGING_PAGE_PRESENT(pte))
//   {
//     if (PAGING_PAGE_SWAPPED(pte))
//     {
//       int swp_type = PAGING_GET_SWPTYPE(pte);
//       int swp_offset = PAGING_GET_SWPOFF(pte);

//       int newfpn;
//       if (MEMPHY_get_freefp(caller->mram, &newfpn) != 0)
//         return -1;

//       __swap_cp_page(caller->active_mswp, swp_offset, caller->mram, newfpn);
//       pte_set_fpn(&caller->mm->pgd[pgn], newfpn);
//     }
//     else
//     {
//       return -1; // page chưa có trong RAM và không bị swap
//     }
//   }

//   int fpn = PAGING_GET_FPN(caller->mm->pgd[pgn]);
//   int phy_addr = fpn * PAGING_PAGESZ + offset;

//   return MEMPHY_read(caller->mram, phy_addr, data);
// }

// int write_mem(struct pcb_t *caller, addr_t dest, BYTE data)
// {
//   int pgn = PAGING_PGN(dest);
//   int offset = PAGING_OFF(dest);
//   uint32_t pte = caller->mm->pgd[pgn];

//   if (!PAGING_PAGE_PRESENT(pte))
//   {
//     if (PAGING_PAGE_SWAPPED(pte))
//     {
//       int swp_type = PAGING_GET_SWPTYPE(pte);
//       int swp_offset = PAGING_GET_SWPOFF(pte);

//       int newfpn;
//       if (MEMPHY_get_freefp(caller->mram, &newfpn) != 0)
//         return -1;

//       __swap_cp_page(caller->active_mswp, swp_offset, caller->mram, newfpn);
//       pte_set_fpn(&caller->mm->pgd[pgn], newfpn);
//     }
//     else
//     {
//       return -1; // page chưa có trong RAM và không bị swap
//     }
//   }

//   int fpn = PAGING_GET_FPN(caller->mm->pgd[pgn]);
//   int phy_addr = fpn * PAGING_PAGESZ + offset;

//   return MEMPHY_write(caller->mram, phy_addr, data);
// }

// #endif
