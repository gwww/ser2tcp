extern uv_buf_t* create_uv_buf_with_data(char *, size_t);
void alloc_buffer(uv_handle_t*, size_t, uv_buf_t*);

#ifdef WIN32
typedef HANDLE serial_handle;
#else
typedef int serial_handle;
#endif
extern int set_serial_attribs(serial_handle, int);
extern void hex_dump(const char* prefix, const void* data, size_t size);
extern char* setup_control_string(char* str);

extern int DebugLevel;

#ifdef _DEBUG
#define dprintf(debug_level, ...) {             \
  if (debug_level <= DebugLevel) do {           \
    printf("[%s:%d] ", __func__, __LINE__);     \
    printf(__VA_ARGS__);                        \
    printf("\n");                               \
    fflush(stdout);                             \
  } while(0);                                   \
}
#else
#define dprintf(...) {}
#endif