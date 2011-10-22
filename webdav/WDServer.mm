//
//  WebdavServer.m
//  Test
//
//  Created by Wouter van de Molengraft on 10/22/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "WDServer.h"
#import "server/webdavserver.h"
@implementation WDServer

- (id) initWithRoot:(NSString *)root andPort:(int)port andUsername:(NSString *)username andPassword:(NSString *)password
{
	self = [super init];
	if (self)
	{
		server = new WebdavServer(
			[root cStringUsingEncoding:NSUTF8StringEncoding], 
			80, 
			[username cStringUsingEncoding:NSUTF8StringEncoding], 
			[password cStringUsingEncoding:NSUTF8StringEncoding]
		);
	}
	
	return self;
}

- (void) dealloc
{
	delete (WebdavServer*) server;
}

- (BOOL) startDaemon
{
	return ((WebdavServer*)server)->startDaemon() ? YES : NO;
}

- (void) stopDaemon
{
	((WebdavServer*)server)->stopDaemon();
}

@end
