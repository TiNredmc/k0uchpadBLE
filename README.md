# k0uchpadBLE [W.I.P]

k0uchpadBLE is a portable Bluetooth low energy keyboard and trackpad. Based on Blackberry Passport keyboard, which capable of touch sensnig via Synaptics s3501 or s3508 chip. Using nRF51822 with Arduino core as a Bluetooth HID over GATT.

Project page
= 

[Hackday.io](https://hackaday.io/project/184529-reverse-engineering-bb-passport-keyboard)

Arduino configuration *IMPORTANT*
=

1. Use Soft device version S130
2. Select low frequency clock as "Sythesized"

Known Issue(s)
=
1. HID report rate is noticeably slow at first bluetooth connection *Solved by reconnect the bluetooth*
