SSD1306CalGasMenuRenderer::SSD1306CalGasMenuRenderer(SSD1306Wire* display, DataSource* dataSource, Calvalue* calvalue, GasManager* gasManager) : SSD1306MenuRenderer(display),
                                                                                                                                                 m_dataSource(dataSource),
                                                                                                                                                 m_calvalue(calvalue),
                                                                                                                                                 m_gasManager(gasManager)
{

}
void SSD1306CalGasMenuRenderer::render(Menu* menu)
{ m_display->clear();
    m_dataSource->readAllValues();

    m_display->setColor(WHITE);
    m_display->setTextAlignment(TEXT_ALIGN_LEFT);
    m_display->setFont(ArialMT_Plain_10);
    m_display->setTextAlignment(TEXT_ALIGN_CENTER);
    m_display->drawString(64, 0, "ALM H LOG");
    m_display->drawLine(0, 14, 128, 14);
    m_display->drawLine(0, 39, 128, 39);
    m_display->drawLine(64, 14, 64, 64);
    int co_value = m_dataSource->getCalibratedValue(0);
    int h2s_value = m_dataSource->getCalibratedValue(1);
    float o2_value = m_dataSource->getCalibratedValue(2);
    float ch4_value = m_dataSource->getCalibratedValue(3);
    int calvalue = m_calvalue->getSelectedCalvalue();
//O2
    m_display->setFont(ArialMT_Plain_10);
    m_display->drawString(9, 17, "O2"); // Unit
    m_display->drawString(12, 27, "%VOL"); // Unit
    m_display->setFont(ArialMT_Plain_16);
    m_display->drawString(44, 18, String(o2_value, 1));
    //CO
    m_display->setFont(ArialMT_Plain_10);
    m_display->drawString(75, 17, "CO"); // Unit
    m_display->drawString(75, 27, "ppm"); // Unit
    m_display->setFont(ArialMT_Plain_16);
    m_display->drawString(107, 18, String(co_value));
    //H2S
    m_display->setFont(ArialMT_Plain_10);
    m_display->drawString(10, 42, "H2S"); // Unit
    m_display->drawString(10, 52, "ppm"); // Unit
    m_display->setFont(ArialMT_Plain_16);
    m_display->drawString(44, 44, String(h2s_value));
    //CH4
    m_display->setFont(ArialMT_Plain_10);
    m_display->drawString(78, 42, "CH4"); // Unit
    m_display->drawString(79, 53, "%LEL"); // Unit
    m_display->setFont(ArialMT_Plain_16);
    m_display->drawString(107, 44, String(ch4_value, 1));
    m_display->display();
    delay(100); // Adjust this delay based on your application's requirements

}