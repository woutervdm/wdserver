//
//  WebdavServer.h
//  Test
//
//  Created by Wouter van de Molengraft on 10/22/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface WDServer : NSObject
{
	void* server;
}

- (id) initWithRoot:(NSString*) root andPort:(int) port andUsername: (NSString*) username andPassword: (NSString*) password;
- (BOOL) startDaemon;
- (void) stopDaemon;

@end
