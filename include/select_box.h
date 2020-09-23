#include <TFT_eSPI.h>

class TFT_Select_Box
{
    private:
    int16_t m_x, m_y, m_w, m_h, m_outlinecolor, m_fillcolor, m_textcolor, m_selectcolor;
    uint8_t m_textsize, m_textdatum;
    TFT_eSPI *m_tft;
    bool m_laststate, m_currstate;

    public:
    bool m_selected;
    String *m_label;
    
    TFT_Select_Box(void);

    void init(TFT_eSPI *gfx, int16_t x, int16_t y, int16_t w, int16_t h,
              uint16_t outline, uint16_t fill, uint16_t textcolor, uint16_t selectcolor,
              String *label, uint8_t textsize);

    void draw();

    void press(bool p)
    {
        m_laststate = m_currstate;
        m_currstate = p;
    }
    bool contains(int16_t x, int16_t y)
    {
        return ((x >= m_x) && (x < (m_x + m_w)) &&
                (y >= m_y) && (y < (m_y + m_h)));
    }

    bool isPressed() { return m_currstate; }
    bool isSelected() { return m_selected; }
    bool justPressed() { return (m_currstate && !m_laststate); }
    bool justReleased() { return (!m_currstate && m_laststate); }
};