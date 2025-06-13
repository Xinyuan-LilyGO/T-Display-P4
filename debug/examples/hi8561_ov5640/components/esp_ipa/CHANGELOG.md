## unReleased

- Add extended configuration to set hue, brightness and statistics window

## 0.3.0~1

- Fix the compiling error with esp-idf v5.4-

## 0.3.0

- Add ATC to configure sensor AE target level by JSON configuration
- AGC different directions of gain and exposure adjustment support different speed parameters
- AGC supports the all exposure range in the part anti-flicker mode
- ACC supports LSC JSON configuration
- ACC adds CCM model 1 to calculate CCM by interpolation algorithm
- ADN JSON configuration supports to generate Gaussian matrix by sigma parameter
- AWB supports to configure statistics parameters
- Pipeline global variables support buffer pointer
- Pipeline supports to add customized IPA node

## 0.2.0

- Added auto color correction algorithm for ISP image color management module
- Added auto denoising algorithm for ISP image denoising module
- Added auto enhancement algorithm for ISP image enhancement module
- Added auto exposure and gain control algorithm for sensor exposure and gain control module
- Added image analysis algorithm to analyze image color temperature and luma
- Added pipeline global variable management functions
- Added script to transform JSON configuration file to C source code for algorithms
- Modified ESP-IDF version from v5.3 to v5.4 to support full ISP modules

## 0.1.0

- Initial version for esp_ipa component

### Enhancements

- Gray world algorithm for auto white balance
- Luma threshold algorithm for auto gain control

