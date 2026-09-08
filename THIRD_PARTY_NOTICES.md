# Third-Party Dependencies and Licensing

VSTBox is an independent project. The MIT license in this repository applies to original VSTBox code and documentation unless a file states otherwise. It does **not** relicense third-party software, SDKs, plug-ins, libraries, assets, or other dependencies.

## Steinberg VST 3 SDK

The Steinberg VST 3 SDK is fetched separately by the development scripts and is intentionally not vendored into this repository. VST 3 SDK version 3.8 and later is distributed under the MIT License. Some components included with or referenced by the SDK, including VSTGUI and certain example code, may carry separate license notices; those notices remain controlling for those components.

VSTBox does not claim ownership of the VST 3 SDK, VST trademarks, or Steinberg materials. Use of the VST name or logo is subject to Steinberg's applicable trademark/usage guidelines.

Official licensing information:
- https://steinbergmedia.github.io/vst3_dev_portal/pages/VST%2B3%2BLicensing/VST3%2BLicense
- https://steinbergmedia.github.io/vst3_dev_portal/pages/VST%2B3%2BLicensing/Which%2Bfiles%2Bfall%2Bunder%2Bwhich%2Blicense.html

## Plug-ins

No third-party VST plug-ins are distributed as part of VSTBox unless explicitly stated otherwise in the future.

Users and contributors are responsible for ensuring that any plug-in they install, test, convert, benchmark, package, or otherwise use with VSTBox is used in accordance with that plug-in's license, EULA, redistribution restrictions, copy-protection requirements, and other applicable terms.

A VSTBox compatibility result or benchmark does not grant a license to a plug-in and does not imply endorsement, affiliation, or certification by the plug-in developer.

## Other dependencies

Other third-party dependencies remain subject to their own licenses. Where VSTBox fetches dependencies during setup or build steps, those dependencies should remain outside the repository unless their license permits redistribution and the required notices are preserved.
