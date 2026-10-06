# Analysis Report



for the language and comarison here need ispecify ang hidden cost that the language pays for having that and where would you notice it
I asked for the cost daw is, where do you notice ang cost when u use implementation ng category sa language vs sa C

1.
| Category | What the language gives for free | What cost it pays | Where you'd notice it in the language vs. in C |
|----------|--------------------------|----------------------------|---------------------------------|
| Strings | In Java, the String type tracks its own length, allows any character including an embedded '\0', and manages its memory for you. |Strings are immutable, so every append creates a new string and copies all the old characters into it. Appending one character 100,000 times means copying a longer and longer string each time, which adds up to billions of characters copied in total. The discarded intermediate strings also pile up as garbage for the collector to clean up. | You notice it when you build a long string one character at a time with += in a loop. Every append copies the whole string so far, so after something like 100,000 appends the program gets visibly slow. StringBuilder avoids this by keeping extra room in its buffer and only making a bigger one when that room runs out. The implementation I made for dt_str_append works the same way. If the new bytes fit in the spare capacity, it just memcpys them onto the end, and it only reallocs (and copies the old bytes) when the buffer is full. |
2.
| Category | What the language gives for free | What cost it pays | Where you'd notice it in the language vs. in C |
|----------|--------------------------|----------------------------|---------------------------------|
| Array | Python has a type called "List" but it functions more like a dynamic array than the standard ADT List, it is automatically resizing and doesn't need to be declared to hold a specific data type like in C with char or int. |  Flexibility isn't free. Resizing means occasionally allocating a bigger block and copying every element, and the spare capacity wastes memory. | You notice this when coding in C when you build an array, it has to be a fixed size with an explicitly declared type. First of all, this is very annoying to use and easy to forget especially if you freshly migrated from python. In python, you usually notice this in runtime errors when an array is accidentally filled with a string when you're trying to add all of its elements arithmetically, you can also notice the trade-off of better flexibility in the speed of program executions. |
3.
| Category | What the language gives for free | What cost it pays | Where you'd notice it in the language vs. in C |
|----------|--------------------------|----------------------------|---------------------------------|
| Int | Python's int has arbitrary precision. It grows as large as you need, so 2**100 or a huge factorial just works, and you never think about overflow. | Memory and speed overhead. Every int in python is large from the start, so even a small number takes about 28 bytes (versus 4 for a C int). |Python ints never overflow, so you never think about size. In C, an int is fixed at 32 bits (the infamous 2,147,483,647), and going past that overflows, which is undefined behavior for signed ints. You have to choose the right type up front (long long, unsigned). |


## 2.  What does the C version let you do that a compiler enforcing the check wouldn't, and is any of it worth wanting?
- **What C allows:** C lets me read any member of the union whether or not the tag matches. The same is true in dt_ref.c: even though the reference is responsible for releasing its cell, nothing in C stops me from using the cell after dt_ref_release. 
- **Is it worth wanting?** Honestly, it depends. It is good for truly learning how memory works. The downside however is that correctness depends on me remembering the check. A compiler that enforces tagged unions and tracks ownership is generally better for a forgetful person like myself, as forgetting results in errors from the compiler.

## 3. Map Design
- **Variables**
    - `dt_map` {  
        - *buckets*: an array of dt_entry(s),  
        - *nbuckets*: tracking the number of buckets,  
        - *order*: an array that tracks the insertion order,  
        - *lenkeys*: which tracks the number of inserted keys  
    }  
    - `dt_entry` {  
        - *key*: the hashed key,  
        - *value*: a pointer to the environment-owned value,  
        - *next*: a pointer to the next entry in the chain  
}  
- **Specifications**
  - **Lookup:** Hash the key (64-bit FNV-1a, mod *nbuckets*) to pick a bucket, then search that chain with *strcmp* to find, if it exists, a matching key.
  - **Put:** An existing key is overwritten in place and keeps its position. A new key is copied, linked into its chain, and appended to *order*.
  - **Remove:** The entry is unlinked, its slot in *order* is removed by shifting later keys left, and the key and entry are freed.
  - **Fixed Size:** The map has a limited nbuckets of exactly 16, and a key limit of 1024 (amply large numbers to avoid resizing).

## 4. Access after release vs. an unreleased allocation
- **Access after release:** in dt_ref.c, the cell is freed when the reference is released, so using it afterwards would read memory that may now belong to something else. In a long-running server this can corrupt data or crash at a random later time, which is hard to trace. The implemented released flag in here stops it. So dt_ref_borrow and dt_ref_release check it and return  the status: DT_ERR_RELEASED instead of touching the cell. In a command-line tool it is still a bug, since it can crash or give wrong output.

- **Unreleased allocation:** if the reference is never released, the cell stays allocated and nothing is corrupted. In a long-running server the memory keeps growing until it slows down or gets killed. The driver catches it at the end by reading the flag with dt_ref_is_released. It then returns the tag DT_ERR_LEAK. In a command-line tool that exits in a second, the operating system takes the memory back anyway, so it's not that harmful.
