

#include "Python.h"
#include "osdefs.h"

#include <string.h>
#include <assert.h>

#ifdef __cplusplus
  extern "C" {
#endif

char* Py_GetProgramName(void);


char* PATH_OVERRIDE;

char* Py_GetPrefix()
{
    static char prefix[MAXPATHLEN+1];

    if (prefix[0] != 0)
        return prefix;

    char* name = Py_GetProgramName();
    size_t len = strlen(name);
    char * end = &name[len-1];
    while (*end != '/')
    {
        --len;
        --end;
    }
    
    assert(*end == '/');
    ++end;
    if (strncmp(end, "python", strlen("python")) == 0)
    {
        strncat(prefix, name, len);
        strcat(prefix, "Lib");
    }
    else
    {
        // assume newsflash
        strncat(prefix, name, len);
        strcat(prefix, "Python-2.5.1/Lib");
    }
    return prefix;
}

char* Py_GetExecPrefix()
{
    static char prefix[MAXPATHLEN+1];

    if (prefix[0] != 0)
        return prefix;

    char* name = Py_GetProgramName();
    size_t len = strlen(name);
    char * end = &name[len-1];
    while (*end != '/')
    {
        --end;
        --len;
    }
    
    assert(*end == '/');
    ++end;
    if (strncmp(end, "python", strlen("python")) == 0)
    {
        strncat(prefix, name, len);
        strcat(prefix, "DLLs");
    }
    else
    {
        // assume newsflash
        strncat(prefix, name, len);
        strcat(prefix, "Python-2.5.1/DLLs");
    }
    return prefix;    
}

char* Py_GetProgramFullPath()
{
    return Py_GetProgramName();
}


char* Py_GetPath(void)
{
    if (PATH_OVERRIDE)
        return PATH_OVERRIDE;

    static char path[MAXPATHLEN+1];
    if (path[0] != 0)
        return path;
    
    const char* prefix = Py_GetPrefix();
    const char* exec   = Py_GetExecPrefix();
    const char* name   = Py_GetProgramName();
    size_t       len   = strlen(name);
    const char*  end   = &name[len-1];
    while (*end != '/')
    {
        --end;
        --len;
    }
    strncat(path, name, len-1);
    strcat(path, ":");
    
    strcat(path, prefix);
    strcat(path, ":");
    strcat(path, exec);
    return path;

}

void Py_OverridePath(char* override)
{
    PATH_OVERRIDE = override;
}

#ifdef __cplusplus
  }
#endif
