/* 
        Block 1
       /       \
      A         B
      |         |
   use x      use x
   ...         |
   def x       |
      \        /
         C
         |
       use x

 * Note that we build interval so that if x is live
 * on exit, its liveness must extend through the exit
 * even if there's a def in between.
 */

