#include "vcWebFile.h"

#include <UIKit/UIKit.h>

void vcWebFile_OpenBrowser(const char *pWebpageAddress)
{
  NSString *pWebpageAddressNS = [NSString stringWithCString:pWebpageAddress encoding:NSUTF8StringEncoding];
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:pWebpageAddressNS] options:@{} completionHandler:nil];
}
