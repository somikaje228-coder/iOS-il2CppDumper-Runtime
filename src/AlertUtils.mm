#include "AlertUtils.h"

UIViewController* getTopViewController(void) {
    for (UIWindow *window in [UIApplication sharedApplication].windows) {
        if (!window.isHidden && window.rootViewController) {
            UIViewController *top = window.rootViewController;
            while (top.presentedViewController) {
                top = top.presentedViewController;
            }
            return top;
        }
    }
    return nil;
}

void showWaiting(NSString *msg, UIAlertController* __strong *alert) {
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_main_queue(), ^{
        *alert = [UIAlertController alertControllerWithTitle:ALERT_TITLE
                                                    message:[NSString stringWithFormat:@"\n%@\n\n", msg]
                                             preferredStyle:UIAlertControllerStyleAlert];

        UIActivityIndicatorView *indicator = [[UIActivityIndicatorView alloc]
            initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
        indicator.translatesAutoresizingMaskIntoConstraints = NO;
        [indicator startAnimating];
        [(*alert).view addSubview:indicator];

        [NSLayoutConstraint activateConstraints:@[
            [indicator.centerXAnchor constraintEqualToAnchor:(*alert).view.centerXAnchor],
            [indicator.bottomAnchor constraintEqualToAnchor:(*alert).view.bottomAnchor constant:-16]
        ]];

        UIViewController *topVC = getTopViewController();
        if (topVC) {
            [topVC presentViewController:*alert animated:YES completion:^{
                dispatch_semaphore_signal(sem);
            }];
        } else {
            NSLog(@"[Dumper] showWaiting: no VC available");
            dispatch_semaphore_signal(sem);
        }
    });
    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC));
}

void dismissWaiting(UIAlertController *alert) {
    if (!alert) return;
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_main_queue(), ^{
        [alert dismissViewControllerAnimated:YES completion:^{
            dispatch_semaphore_signal(sem);
        }];
    });
    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC));
}

void showSuccess(NSString *msg) {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithFormat:@"✅ %@", ALERT_TITLE]
                                                                      message:msg
                                                               preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:nil]];

        UIViewController *topVC = getTopViewController();
        if (topVC) {
            [topVC presentViewController:alert animated:YES completion:nil];
        } else {
            NSLog(@"[Dumper] showSuccess: no VC available");
        }
    });
}

void showInfo(NSString *msg, float duration) {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithFormat:@"ℹ️ %@", ALERT_TITLE]
                                                                      message:msg
                                                               preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:nil]];

        UIViewController *topVC = getTopViewController();
        if (topVC) {
            [topVC presentViewController:alert animated:YES completion:nil];

            if (duration > 0) {
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(duration * NSEC_PER_SEC)),
                    dispatch_get_main_queue(), ^{
                        if (alert.presentingViewController) {
                            [alert dismissViewControllerAnimated:YES completion:nil];
                        }
                    });
            }
        } else {
            NSLog(@"[Dumper] showInfo: no VC available");
        }
    });
}

void showError(NSString *msg) {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithFormat:@"❌ %@", ALERT_TITLE]
                                                                      message:msg
                                                               preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:nil]];

        UIViewController *topVC = getTopViewController();
        if (topVC) {
            [topVC presentViewController:alert animated:YES completion:nil];
        } else {
            NSLog(@"[Dumper] showError: no VC available");
        }
    });
}