#ifndef VERIFY_H_
#define VERIFY_H_

void verify_failed(char *message, char *filename, int line);

/*
  Abort with a message if condition x is not true.
  This is like assert() but is not disabled in Release mode.
*/
#define verify(X) if (!(X)) verify_failed( #X , __FILE__ , __LINE__)

/*
  Jump to cleanup block if return code is not zero
*/
#define check(x) do {if((x) != 0) goto cleanup;} while(0)

#endif
