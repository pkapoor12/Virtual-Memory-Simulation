Pujeet K
Virtual Memory Simulation

This project simulates the virtualization of memory by an operating system to its processes. In this simple implementation, only 4 running processes are considered as part of the
simulation. To represent the physical memory and virtual address spaces, a 64-byte array is used for simplicity. The operating system uses the popular method of paging to
virtualize its memory efficiently. It handles 3 commands: map, load, and store. Map assigns a virtual page to physical frame, store places a value at a particular address (in this
case, an index into the memory array), and load reads a value at an address. To support multiple processes having multiple pages in memory, the OS can evict pages by writing them
to disk (in this case, a binary file "disk.bin"). To handle a page fault, the OS reads in the sought-after page from disk into memory (array of bytes). An address can only store
values from 0 to 255, as they only hold one byte. To keep track of pages, this project uses a simple implementation of a linear page table, storing up to 4 entries for each 16-byte
page. The entries of the page table are all 8-bit integers representing a bit field. The top 2 bits indicate the virtual page number, the next 2 indicate the physical frame number,
and the next 4 indicate certain statuses of the page, such as tracking read/write permissions, tracking whether that page is present in memory or in disk, whether it has been
referenced, and whether its valid. This project really only manages the first two mentioned statuses, for simplicity's sake.

NOTE: My implementation stores all the page tables in physical frame 0, as each page table is only 4 bytes long and there are 4 page tables, so only 16 bytes are need (1 page).
Although this would not scale up to more than 4 processes, I did it this way is to avoid any cascading effects when handling
page faults and reduce internal fragmentation. When a page is chosen to be evicted, it can never be frame 0, as all the page tables for each process is stored there. That way, pages can be swapped out and page
table entries can be updated seamlessly without a cascade, since a frame with a page table cannot be evicted. For scaling past 4 processes, it would be advisable to not store all page tables in one page, and implement a different page-eviction policy, such as Least Recently Used or Least Frequently Used, instead of the Round Robin algorithm implemented here.

Credit for the creation of the starter code goes to WPI staff of CS3013 C term. The starter code includes the implementation of boiler plate functions in the mmu.c, memsim.c, and input.c files, as well as writing all the header files.
