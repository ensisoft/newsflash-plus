

#include "Python.h"
#include "osdefs.h"

#include <string.h>
#include <assert.h>

#ifdef __cplusplus
  extern "C" {
#endif

#ifdef PY_WINDOWS
#define SEP '\\'
#endif
#ifdef PY_LINUX
#define SEP '/'
#endif

char* Py_GetProgramName(void);

char* Py_GetPrefix()
{
    static char prefix[MAXPATHLEN+1];
    char* name;
    size_t len;
    size_t i;
    char sep = SEP;

    if (prefix[0] != 0)
        return prefix;
    
    name = Py_GetProgramName();
    len  = strlen(name);
    for (i=len-1; i>0; --i)
    {
        if (name[i] == SEP)
        {
            ++i;
            break;
        }
    }
    if (!strncmp(&name[i], "python", strlen("python")))
    {
        strncat(prefix, name, len - (len-i));
        if (i > 0)
            strncat(prefix, &sep, 1);
        strcat(prefix, "Lib");
    }
    else
    {
        // assume newsflash
        strncat(prefix, name, len - (len-i));
        strcat(prefix, "Python-2.5.1");
        strncat(prefix, &sep, 1);
        strcat(prefix, "Lib");
    }
    return prefix;
}

char* Py_GetExecPrefix()
{
    static char prefix[MAXPATHLEN+1];
    char* name;
    size_t len;
    size_t path;
    int i;
    char sep = SEP;

    if (prefix[0] != 0)
        return prefix;
    
    name = Py_GetProgramName();
    path = 0;
    len  = strlen(name);
    for (i=len-1; i>0; --i)
    {
        if (name[i] == SEP)
        {
            ++i;
            break;
        }
    }
    if (!strncmp(&name[i], "python", strlen("python")))
    {
        strncat(prefix, name, len - (len-i));
        if (i > 0)
            strncat(prefix, &sep, 1);
        strcat(prefix, "DLLs");
    }
    else
    {
        // assume newsflash
        strncat(prefix, name, len - (len-i));
        strcat(prefix, "Python-2.5.1");
        strncat(prefix, &sep, 1);
        strcat(prefix, "DLLs");
    }
    return prefix;
}

char* Py_GetProgramFullPath()
{
    return Py_GetProgramName();
}


char* Py_GetPath(void)
{
    static char path[MAXPATHLEN+1];
    const char* prefix; 
    const char* exec;
    const char* name;
    char delim = DELIM;
    size_t i;
    size_t len;

    if (path[0] != 0)
        return path;

    prefix = Py_GetPrefix();
    exec   = Py_GetExecPrefix();
    name   = Py_GetProgramName();
    
    len = strlen(name);
    for (i=len-1; i>0; --i)
    {
        if (name[i] == SEP)
            break;
    }
    strncat(path, name, len - (len-i));
    strncat(path, &delim, 1);
    strcat(path, prefix);
    strncat(path, &delim, 1);
    strcat(path, exec);
    
    return path;
}

#ifdef __cplusplus
  }
#endif
