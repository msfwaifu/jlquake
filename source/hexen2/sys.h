// sys.h -- non-portable functions

//
// system IO
//
void Sys_Error(const char* error, ...);
// an error will cause the entire program to exit

void Sys_Quit(void);
