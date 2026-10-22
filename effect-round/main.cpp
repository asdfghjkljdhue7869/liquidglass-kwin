#include "liquidglassround.h"

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED(LiquidGlassRoundEffect,
                                      "metadata.json",
                                      return LiquidGlassRoundEffect::supported();
                                      ,
                                      return LiquidGlassRoundEffect::enabledByDefault();)

}

#include "main.moc"
