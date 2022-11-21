# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added 
### Fixed 
### Pending
 - keep google for sync only until it's fixed by Google team (as it can't see user files even in app folder)
 - support koofr api
 - use api credentials from config (multiple api support) -> modify isUserAuthenticated() to init OAuth2 variable
 - refactor: use bearer oauth class header method instead of manual input
 - implement download only mode (by default from app folder, i.e. from gamesave sync)
 - add support for mode selection
    - sync_only     - backup/restore gamesave data
    - download_only - download data from cloud 
        - config must have download folder parameter
            - if default is set (app folder name), then restore missing gamesave data from cloud
            - otherwise it's full download mode: get all files from configured cloud folder 
                - add check for free space (if possible)           
    - sync_upload   - first two modes combined 

 - s32 sysFsGetFreeSize(const char *path, u32 *blockSize, u64 *freeBlocks); <- check how to get free space (https://github.com/nevik-xx/psl1ght/blob/master/include/sys/sysfs.h)
 https://stackoverflow.com/questions/1449055/disk-space-used-free-total-how-do-i-get-this-in-c
    u64 free_space = ((u64)vfs.f_bsize * vfs.f_bfree); // in bytes
    I guess the total space would be using vfs.f_blocks;

## [1.1.8] - 2022-11-18
### Added
 - finally app verified by google
 - found issue tracker  https://issuetracker.google.com/issues/111221778
 
## 221111
 - update demo video for google verification
 - sent feedback (complaint) to google about drive.file limitation bug:
   I believe that OAuth2 for limited devices has too limited scopes. 
   Basically because drive.file scope doesn't works as expected.
     Application have a dedicated folder on Google and user is aware of that. I believe that everything manually saved there  by user should be available to app which is a real owner of that folder. Or at least there should be a respective scope, i.e. drive.app - where application owns all files (have full control) in dedicated folder, including manually added files and folders. 
   Otherwise there is no point for user to put there anything and it should be forbidden. 

## 221110
 - continuing app verification for drive.readonly sensitive scope
 - finding out that OAuth2 for devices doesn't support that:
    https://developers.google.com/identity/protocols/oauth2/limited-input-device#allowedscopes
 - if google won't fix that, then there is no need for verification
 - rename app to get rid of any possible misleading naming:
    Consistent Branding
    Based on the information you provided for the project and icon as shown to users https://lh3.googleusercontent.com/2Uw6IoZlUPGWcbx3v6ql6qC1UtphHQdm-5B-h3VyzJ_AKvhsDTT6808pEgqRRqKxhivq , your project requests permissions in a manner that may mislead users into mistaking the identity of the application requesting the permission, especially Google Drive.

    To fix this issue, update the icon URL and other relevant content on the OAuth Consent Screen of the Google API Console, making sure that your project accurately represents its true identity to Google users.
    "
 - changed icon/logo and backgrounds;

## 221107 
 - add base64.encode;
 - rest: use base64 basic authorization instead of sending credentials in url;
 - oauth2: get registration url instead of local const;
 - app config: store scope, support for multiple credentials, keep device code even after refresh, store sync ts; 
 - try to update app config file instead of rewriting it from scratch;
 - json: pretty print to file;

## Previous
 - add Makefile to run .self in emulator with debug listener (make -f run)
 - workaround to fix directory reading loops (rpcs3 bug)
 - exctract api credentials into non public ('hidden') credentials.h header
Links

- https://zerkman.sector1.fr/ps3/psl1ght/files.html
- https://github.com/nevik-xx/psl1ght/
- https://developers.google.com/drive/api/guides/branding <- guadelines for use of google branding (avoid any for faster veryfication)
- https://creativecommons.org/licenses/by-nc/4.0/ <- free for personal use image license
- https://pngimg.com/image/50232
- https://pngimg.com/image/50125
- https://developers.google.com/terms/api-services-user-data-policy#additional_requirements_for_specific_api_scopes
- https://stackoverflow.com/questions/74233277/drive-api-manually-created-file-is-not-found-by-api-using-same-account-oauth2
- https://www.svgrepo.com/page/licensing <- CCO, This license also might be referred as No copyright or CC0 1.0 Universal PD Dedication on our website.
- https://www.svgrepo.com/svg/74684/game-console
- https://issuetracker.google.com/issues/111221778
 
