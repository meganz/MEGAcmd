#include "MacUtils.h"
#include <Cocoa/Cocoa.h>

bool downloadFileSynchronously(string url, string path)
{
    NSString *stringURL = [NSString stringWithCString:url.c_str() encoding:NSUTF8StringEncoding];
    NSURL *myURL = [NSURL URLWithString:stringURL];

    NSData *data = [NSData dataWithContentsOfURL:myURL];
    if (data == nil)
    {
        return false;
    }

    BOOL result = [data writeToFile:[NSString stringWithCString:path.c_str() encoding:NSUTF8StringEncoding] atomically:NO];
    if (result == NO)
    {
        return false;
    }
    return true;
}
