{
  enable_i2c = {
    name = "enable-i2c";
    dtsText = ''
      /dts-v1/;
      /plugin/;
      / {
        compatible = "brcm,bcm2837";
        fragment@0 {
          target = <&i2c1>;
          __overlay__ {
            status = "okay";
          };
        };
      };
    '';
  };
  enable_audio = {
    name = "enable-audio";
    dtsText = ''
      /dts-v1/;
      /plugin/;
      / {
        compatible = "brcm,bcm2837";
        fragment@0 {
          target = <&audio>;
          __overlay__ {
            status = "okay";
          };
        };
      };
    '';
  };
  googlevoicechat_soundcard = {
    name = "googlevoicechat-soundcard";
    dtsText = ''
      /dts-v1/;
      /plugin/;
      / {
        compatible = "brcm,bcm2837";
        fragment@0 {
          target = <&i2s_clk_producer>;
          __overlay__ {
            status = "okay";
          };
        };
        fragment@1 {
          target = <&gpio>;
          __overlay__ {
            googlevoicehat_pins: googlevoicehat_pins {
              brcm,pins = <16>;
              brcm,function = <1>; /* out */
              brcm,pull = <0>; /* up */
            };
          };
        };
        fragment@2 {
          target-path = "/";
          __overlay__ {
            voicehat-codec {
              #sound-dai-cells = <0>;
              compatible = "google,voicehat";
              pinctrl-names = "default";
              pinctrl-0 = <&googlevoicehat_pins>;
              sdmode-gpios= <&gpio 16 0>;
              status = "okay";
            };
          };
        };
        fragment@3 {
          target = <&sound>;
          __overlay__ {
            compatible = "googlevoicehat,googlevoicehat-soundcard";
            i2s-controller = <&i2s_clk_producer>;
            status = "okay";
          };
        };
      };
    '';
  };
}