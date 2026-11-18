#include "lgwobble.h"

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED(LiquidGlassWobbleEffect,
                                      "metadata.json",
                                      return LiquidGlassWobbleEffect::supported();
                                      ,
                                      return false;)

}

#include "main.moc"
