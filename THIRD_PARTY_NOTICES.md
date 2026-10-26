# Third-party notices

SideCurve source is distributed under the MIT license in LICENSE. Third-party
components retain their own licenses and copyright notices.

- Steinberg VST3 SDK, pinned revision
  `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`: MIT, see licenses/VST3-SDK.txt.
- VSTGUI: BSD-style license, see licenses/VSTGUI.txt.
- VSTGUI's miniz and RapidJSON build dependencies: their upstream notices are
  retained in licenses/VSTGUI-miniz.txt and licenses/VSTGUI-RapidJSON.txt. Some
  built library objects need not be linked into a particular final plugin binary.
  RapidJSON's upstream notice also describes JSON_checker; that test utility is
  not a SideCurve dependency and is not redistributed by this project.
- The macOS build links the system Expat library; no Expat library binary is
  copied into the release. Other platforms must review their static dependencies.
- Filter coefficient equations are based on Robert Bristow-Johnson's Audio EQ
  Cookbook, published by W3C: https://www.w3.org/TR/audio-eq-cookbook/ .
- The REAPER host interface ABI is documented by Cockos:
  https://github.com/justinfrankel/reaper-sdk/blob/main/sdk/reaper_vst3_interfaces.h .
  SideCurve declares the interoperable interface; it does not bundle REAPER.

VST is a trademark of Steinberg Media Technologies GmbH. REAPER is a Cockos
product. No endorsement by either company is implied. No third-party fonts,
Kickstart/Cableguys graphics, or Kickstart binaries are shipped in the package.

This notice is not a trademark, patent or design clearance for SideCurve or its logo.
