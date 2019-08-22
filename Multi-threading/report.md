** Yejee Jenny Lee **
** April 8th, 2019 **

We basically have two ways to index: with either the TypeIndex (where we use SetOfMovies, indexing the movies by a field and storing them in a LinkedList of Movie structs), or the OffsetIndex (where we use MovieSet, and store just the DocIds and RowIds of where the movies are stored on disk).


Start by clarifying your hypotheses:

- Which is bigger, the TypeIndex or the OffsetIndex? Why?
  - TypeIndex is bigger than OffsetIndex. I think TypeIndex need more memory because it is storing all movies and their information in the linkedlist. But, OffsetIndex only contains docId and and rowId of where the movies are stored in the disk. 

 - Which will be faster to query, the OffsetIndexer or the TypeIndex? For what kinds of queries? Why?
    - TypeIndex would be faster. Since it contains all the movie, its faster to query. However, on the other hand, OffSetIndex only contains docId and the file path that In order to get the info, we have to access to file path then we have to go through information.
   
- Which will be faster to build, the TypeIndex or the OffsetIndex? Why?
    - TypeIndex is faster to build. One reason why TypeIndex is faster is because of the resize() function. Because we are resizing hashtable all the time, it makes faster to build. Also, TypeIndex only contains 10 entries whereas OffSetIndex contains 22197 entries. 


To make the query comparison fair, let's define some sample queries:

- What movies have Seattle in the title and are Rom/Coms?
  - How long does it take to index? (time)
    - It took 0.109901 seconds to build the OffSetIndex.
    - It took 0.032073 seconds to build TypeIndex.
  - How big is the resulting index? (space)
    - for building and running query on OffSetIndex:
      	  - Cur Real Mem: 27900
	  - Peak Real Mem: 27900
	  - Cur VirtMem: 80452
	  - PeakVirtMem: 145988

    - for building and running query on TypeIndex:
          - Cur Real Mem: 33156
          - Peak Real Mem: 33156
          - Cur VirtMem: 88504
          - PeakVirtMem: 145988
	  
  - How fast/slow is it to make a query and get results? (time)     
    - for OffSetIndex: it took 0.000527 seconds to execute.
    - for TypeIndex: it took 0.000778 seconds to execute.


Part2) comparing it with Data directory because the directory was too huge, I cut down to 20 files in data directory.
1) running in multithread, it takes longer time to run query.
   - for OffSetIndex : Took 0.489243 seconds to execute.
   - for TypeIndex: Took 0.024237 seconds to execute.
2) Also, the memory difference is huge.
   - Memory usage for OffSetIndex query:
   - Cur Real Mem: 987996	Peak Real Mem: 987996 
   - Memory usage for Type Index query:
   - Cur Real Mem: 1656924	Peak Real Mem: 1656924
3) also, building index took really long time.
   - for offset: it took about 7.8 seconds.
   - for typeIndex: it took about 2.33259 seconds.
-----------
Process ID: 8117
Cur Real Mem: 880	Peak Real Mem: 880 	 Cur VirtMem: 6720	PeakVirtMem: 6720

Created DocIdMap
Cur Real Mem: 880	Peak Real Mem: 880 	 Cur VirtMem: 6720	PeakVirtMem: 6720


Building the OffsetIndex
Parsing and indexing files...
22197 entries in the index.
Took 0.109901 seconds to execute. 
Memory usage for OffSetIndex: 
Cur Real Mem: 27900	Peak Real Mem: 27900 	 Cur VirtMem: 80452	PeakVirtMem: 145988

 Find movies that has seattle and is romance/comedy. 
returning movieset
Getting docs for movieset term: "seattle"

There is no movie that has term Seattle and ROM/COM.
count :0 
Took 0.000527 seconds to execute. 
Memory usage for query:
Cur Real Mem: 27900	Peak Real Mem: 27900 	 Cur VirtMem: 80452	PeakVirtMem: 145988
Destroyed OffsetIndex
Cur Real Mem: 25240	Peak Real Mem: 27900 	 Cur VirtMem: 80452	PeakVirtMem: 145988


Building the TypeIndex
10 entries in the index.
Took 0.032073 seconds to execute. 
Memory usage for TypeIndex: 
Cur Real Mem: 33156	Peak Real Mem: 33156 	 Cur VirtMem: 88504	PeakVirtMem: 145988

 Find movies that has seattle and is romance/comedy. 

There is no movie that has term Seattle and ROM/COM.
count: 0
Took 0.000778 seconds to execute. 
Memory usage for query:
Cur Real Mem: 33156	Peak Real Mem: 33156 	 Cur VirtMem: 88504	PeakVirtMem: 145988
Destroyed TypeIndex
Cur Real Mem: 33156	Peak Real Mem: 33156 	 Cur VirtMem: 88504	PeakVirtMem: 145988


Destroyed DocIdMap
Cur Real Mem: 33156	Peak Real Mem: 33156 	 Cur VirtMem: 88504	PeakVirtMem: 145988