
typedef struct lua_State lua_State;

int set_self_binary_path(const char* self_path);
int binject_main_app_has_internal_script();
int binject_main_app_internal_script_handle(lua_State *L, int argc, char **argv);
int luaopen_glua(lua_State* L);

