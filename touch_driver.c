/**
 * @file touch_driver.c
 * @brief 12-Tu� kapasitif dokunmatik tu� tak�m� s�r�c�s�.
 * 
 * I2C tabanl� bir tu� tak�m� cihaz�n� yap�land�r�r, tu� durumlar�n� okur
 * ve bas�ld���nda i�lem yapacak �ekilde tasarl�d�r.
 *
 * Created on: Sep 22, 2021
 * Author: Murat Okumus
 */

#include <string.h>
#include "touch_driver.h"
#include "USER_I2C.h"
#include "USER_GPIO.h"
#include "nrf_drv_twi.h"
#include "nrf_log.h"

/* I2C adresi */
#define KEYPAD_I2C_ADDR    0x50

/* Statik Ayarlar (Kontrolc�ye ilk y�klenir) */
static uint8_t Settings[23] = {
    0xB0, 0x00, 0x00, 0x83, 0xF3, 0x98,
    0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09,
    0xC0, 0xB7
};

static uint8_t IsReadyForKey = 0;

/* Tu� maskelerini temsil eden sabit tablo */
static const uint16_t keyVals[] = {
    0x0002, 0x0200, 0x0800, 0x0400,
    0x0100, 0x0080, 0x0040, 0x0020,
    0x0010, 0x0004, 0x0008, 0x0001
};

/* ---------------------------------------------------------------------------
 * Prototipler
 * --------------------------------------------------------------------------- */
static uint8_t keyp_is_key_down(void);
static uint8_t keyp_is_key_up(void);
static uint16_t keyp_read_key(void);

/* ---------------------------------------------------------------------------
 * Genel Fonksiyonlar
 * --------------------------------------------------------------------------- */

/**
 * @brief  Tu� tak�m� cihaz�n� ilk konfig�re eder.
 */
void keyp_initialize(void) {
    keyp_set_settings();
    keyp_get_settings();
}

/**
 * @brief  Tu� tak�m� durumunu kontrol eder, bir tu� bas�ld���nda i�lem yapar.
 */
void keyp_update(void) {
    if (keyp_is_key_up()) {
        IsReadyForKey = 1;
    } else if (keyp_is_key_down() && IsReadyForKey) {
        IsReadyForKey = 0;

        uint16_t keys = keyp_read_key();
        int count = 0;

        for (int i = 0; i < 12; i++) {
            if ((keys >> i) & 0x01) {
                count++;
            }
        }

        if (count == 1) {
            uint8_t val = ut_get_key_value(keys);
            NRF_LOG_INFO("Basilan tus: %d", val);
        } else {
            keyp_on_key_down(keys, count);
        }
    }
}

/**
 * @brief  Ayar baytlar�n� cihazla payla��r.
 */
void keyp_set_settings(void) {
    int checksum = 0;

    /* Checksum Hesaplama */
    for (int i = 1; i < 22; i++) {
        checksum += Settings[i];
    }
    Settings[22] = (uint8_t)checksum;

    /* Ayarlar� cihazla payla�ma */
    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, KEYPAD_I2C_ADDR, Settings, sizeof(Settings), false);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("keyp_set_settings failed: %d", err_code);
    }
}

/**
 * @brief  Ayar baytlar�n� cihazdan geri okur (test ve do�rulama amac�yla).
 */
void keyp_get_settings(void) {
    for (int i = 0; i < 22; i++) {
        uint8_t writeData[1] = { (uint8_t)(0xB0 + i) };
        uint8_t buf[2] = {0, 0};
        ret_code_t err_code;

        if (!nrf_drv_twi_is_busy(&m_twi)) {
            err_code = nrf_drv_twi_tx(&m_twi, KEYPAD_I2C_ADDR, writeData, sizeof(writeData), false);
            if (err_code != NRF_SUCCESS) {
                NRF_LOG_ERROR("keyp_get_settings TX failed: %d", err_code);
            }
        }

        if (!nrf_drv_twi_is_busy(&m_twi)) {
            err_code = nrf_drv_twi_rx(&m_twi, KEYPAD_I2C_ADDR, buf, sizeof(buf));
            if (err_code != NRF_SUCCESS) {
                NRF_LOG_ERROR("keyp_get_settings RX failed: %d", err_code);
            } else {
                NRF_LOG_INFO("I2C Addr 0x%X --> %d", 0xB0 + i, buf[0]);
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * Dahili Fonksiyonlar
 * --------------------------------------------------------------------------- */

/**
 * @brief  Tu� tak�m� IRQ hatt�n� kontrol eder (d���kse tu� bas�l�).
 */
static uint8_t keyp_is_key_down(void) {
    return (User_Pin_Read(KEYPAD_IRQ_PIN) == 0) ? 1 : 0;
}

/**
 * @brief  Tu� tak�m� IRQ hatt�n� kontrol eder (y�ksekse tu� bas�l� de�il).
 */
static uint8_t keyp_is_key_up(void) {
    return (User_Pin_Read(KEYPAD_IRQ_PIN) != 0) ? 1 : 0;
}

/**
 * @brief  Bas�l� tu�un kodunu I2C'den okuyup d�nd�r�r.
 */
static uint16_t keyp_read_key(void) {
    /* 
       De�erleri cihazdan oku 
       NOT: txDat ve rxDat harici global de�i�kenlerden geliyor
    */
    if (!nrf_drv_twi_is_busy(&m_twi)) {
        (void)nrf_drv_twi_tx(&m_twi, KEYPAD_I2C_ADDR, &txDat, 1, false);
    }
    if (!nrf_drv_twi_is_busy(&m_twi)) {
        (void)nrf_drv_twi_rx(&m_twi, KEYPAD_I2C_ADDR, rxDat, 2);
    }

    uint16_t value = (rxDat[1] << 8) | (rxDat[0]);
    uint16_t keys = 0;

    /* Gelen de�eri bilinen tu� maskelerine �evir */
    switch(value) {
        case 4:    value = 0x0200; break;
        case 2:    value = 0x0800; break;
        case 1:    value = 0x0400; break;
        case 8:    value = 0x0100; break;
        case 16:   value = 0x0080; break;
        case 32:   value = 0x0040; break;
        case 64:   value = 0x0020; break;
        case 128:  value = 0x0010; break;
        case 256:  value = 0x0004; break;
        case 1024: value = 0x0002; break;
        case 512:  value = 0x0008; break;
        case 2048: value = 0x0001; break;
    }

    /* De�erleri bit baz�nda kontrol edip keyVals[] tablosuyla e�le�tir */
    for (uint8_t i = 0; i < 12; i++) {
        if (value & keyVals[i]) {
            keys |= (1 << i);
        }
    }

    return keys;
}

