
typedef struct lua_State lua_State;
void preload_set_pack_hook( int (*pack_hook)(const char* inpath, const char* outpath));
int preload_all(lua_State* L);

