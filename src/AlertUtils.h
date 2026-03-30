#pragma once
#import <UIKit/UIKit.h>

#define ALERT_TITLE @"Dumper Update by Mr D\nDS Gaming"

UIViewController* getTopViewController(void);
void showWaiting(NSString *msg, UIAlertController* __strong *alert);
void dismissWaiting(UIAlertController *alert);
void showSuccess(NSString *msg);
void showInfo(NSString *msg, float duration);
void showError(NSString *msg);