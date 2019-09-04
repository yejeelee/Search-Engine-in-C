# Search-Engine-in-C

1. Implement Doubly LinkList
2. Implement Hashtables
3. Building file indexer
4. Multi-threading
5. Client-Server Application

A search engine has a few components: a file crawler, a file processor, an indexer, and a query processor. The file crawler starts in one place, and traverses to find other files. On the web, this means starting at one page and following all the links on the page; in a file system, it means starting in one directory and traversing through the directory structure until all files have been found. The file processor takes each of those files and finds significant words or concepts in the file. The indexer creates a data structure that (essentially) populates a data structure that makes it easy to find documents that have similar topics, or find documents that contain a certain word. The query processor lets us work with that data structure to get the results based on what we ask for.
