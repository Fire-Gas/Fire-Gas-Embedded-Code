#ifndef ADS1115_SETTING_H
#define ADS1115_SETTING_H

#define ADS1115_REG_CONVERSION    0x00  // 변환된 ADC 값이 저장되는 레지스터
#define ADS1115_REG_CONFIG        0x01  // 장치 설정 레지스터
#define ADS1115_REG_LO_THRESH     0x02  // 비교기 하한 임계값
#define ADS1115_REG_HI_THRESH     0x03  // 비교기 상한 임계값

/* OS (Operational Status / Single-shot conversion start) */
#define ADS1115_OS_SHIFT          15
#define ADS1115_OS_START          (1 << ADS1115_OS_SHIFT)

/* MUX (Input Multiplexer Configuration) - 채널 선택 */
#define ADS1115_MUX_SHIFT         12
#define ADS1115_MUX_SINGLE_0      (0x4 << ADS1115_MUX_SHIFT) // AIN0
#define ADS1115_MUX_SINGLE_1      (0x5 << ADS1115_MUX_SHIFT) // AIN1
#define ADS1115_MUX_SINGLE_2      (0x6 << ADS1115_MUX_SHIFT) // AIN2
#define ADS1115_MUX_SINGLE_3      (0x7 << ADS1115_MUX_SHIFT) // AIN3

/* PGA (Programmable Gain Amplifier) - 증폭 및 전압 범위 */
#define ADS1115_PGA_SHIFT         9
#define ADS1115_PGA_6_144V        (0x0 << ADS1115_PGA_SHIFT)
#define ADS1115_PGA_4_096V        (0x1 << ADS1115_PGA_SHIFT)
#define ADS1115_PGA_2_048V        (0x2 << ADS1115_PGA_SHIFT)

/* MODE (Device Operating Mode) */
#define ADS1115_MODE_SHIFT        8
#define ADS1115_MODE_CONTINUOUS   (0 << ADS1115_MODE_SHIFT)
#define ADS1115_MODE_SINGLE_SHOT  (1 << ADS1115_MODE_SHIFT)

/* DR (Data Rate) - 초당 샘플링 횟수 */
#define ADS1115_DR_SHIFT          5
#define ADS1115_DR_128SPS         (0x4 << ADS1115_DR_SHIFT)

/* COMP_QUE (Comparator Que) - 비교기 비활성화 (기본값) */
#define ADS1115_COMP_QUE_DISABLE  0x0003

#endif