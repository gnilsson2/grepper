A Windows "find in files", somewhat like grep.  
Allways reqursive (like grep -r)  
Allways ingore case (like grep -i)  
Searches in parallel, so quite fast, but can heavely load your cpu, if you search in like C:\  
  
Usage: grepper [options] string  
   --search-in=string     Where to search recursively  
   --exclude-dir=string   Repeat for each directory to exclude  
   string                 Text to be searched for  
Example:  
  grepper --exclude-dir =.git --exclude-dir =.vs --search-in=\"C:\\Users\\gnils\\Documents\\_MyProj\" \"My vi.vi\"  
