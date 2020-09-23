#include "select_box.h"
#define SERIAL_DEBUG
#include "SerialDebug.h"


TFT_Select_Box::TFT_Select_Box(void) : m_x(),
    m_y(0),
    m_w(0),
    m_h(0),
    m_outlinecolor(TFT_BLACK),
    m_fillcolor(TFT_WHITE),
    m_textcolor(TFT_BLACK),
    m_selectcolor(TFT_GREEN),
    m_textsize(1),
    m_textdatum(MC_DATUM),
    m_tft(nullptr),
    m_laststate(false),
    m_currstate(false),
    m_selected(false),
    m_label(nullptr)
{
}

void TFT_Select_Box::init(TFT_eSPI *gfx, int16_t x, int16_t y, int16_t w, int16_t h,
            uint16_t outline, uint16_t fill, uint16_t textcolor, uint16_t selectcolor,
            String *label, uint8_t textsize)
{
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_tft = gfx;
    m_outlinecolor = outline;
    m_fillcolor = fill;
    m_selectcolor = selectcolor;
    m_textcolor = textcolor;
    m_textsize = textsize;
    m_label = label;
}

void TFT_Select_Box::draw()
{
    m_tft->fillRect(m_x, m_y, m_w, m_h, m_fillcolor);
    SerialDebug("selected");
    SerialDebug(m_selected);
    SerialDebug("\n");

    if (m_selected)
    {
        m_tft->drawRect(m_x, m_y, m_w, m_h, m_selectcolor);
    }
    else
    {
        m_tft->drawRect(m_x, m_y, m_w, m_h, m_outlinecolor);
    }

    m_tft->setTextSize(m_textsize);
    m_tft->setTextColor(m_textcolor, m_fillcolor);

    uint8_t tempdatum = m_tft->getTextDatum();
    m_tft->setTextDatum(m_textdatum);
    uint16_t tempPadding = m_tft->padX;
    m_tft->setTextPadding(0);

    m_tft->drawString(*m_label, m_x + (m_w / 2), m_y + (m_h / 2));

    m_tft->setTextDatum(tempdatum);
    m_tft->setTextPadding(tempPadding);
}