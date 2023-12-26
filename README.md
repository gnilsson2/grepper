# A Windows "find in files", somewhat like grep.  
* Allways reqursive (like grep -r)  
* Allways ingore case (like grep -i)  
* Searches in parallel, so quite fast, but can heavely load your cpu, if you search in like C:\  
  
```
Usage: grepper [options] string path
Search for string in all files recursively in path.

   --exclude-dir=string Repeat for each directory to exclude
   string               Text to be searched for
   path                 Where to search recursively

Example:
  grepper --exclude-dir =.git --exclude-dir =.vs "My vi.vi" "C:\Users\gnils\Documents\_MyProj"
```