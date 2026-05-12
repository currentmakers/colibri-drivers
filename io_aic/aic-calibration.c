#include "colibri-io-eeprom.h"
#include <stdio.h>

void eeprom_layout(eeprom_layout_t* layout)
{
    layout->vendor_id = 1;
    layout->vendor_model_id = 0x0001;
    layout->vendor_revision = 0x41;       // 'A'

    layout->vendor_name_ptr =0x200;
    layout->vendor_name_len =13;
    layout->model_name_ptr =0x20E;
    layout->model_name_len =11;

    layout->product_link_ptr =0x21A;
    layout->product_link_len =25;
    layout->doc_link_ptr =0x234;
    layout->doc_link_len =40;

    layout->pic_len;
    memcpy(layout->text_area,
           "CurrentMakers\0"
           "Colibri AIC\0"
           "https://currentmakers.com\0"
           "https://stm32world.com/wiki/Colibri_AIC\0",
           sizeof(layout->text_area)
    );
    layout->pic_arm = code;
}
