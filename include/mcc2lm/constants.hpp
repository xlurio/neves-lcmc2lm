#ifndef MCC2LM_CONSTANTS_HPP

#define MCC2LM_CONSTANTS_HPP

#include <array>
#include <string>

namespace mcc2lm
{
    inline const std::string LCMC_BASEDIR = "assets/Lcmc/data/character";

    inline const std::array<std::string, 15> LCMC_FILENAMES = {
        "LCMC_A.XML",
        "LCMC_B.XML",
        "LCMC_C.XML",
        "LCMC_D.XML",
        "LCMC_E.XML",
        "LCMC_F.XML",
        "LCMC_G.XML",
        "LCMC_H.XML",
        "LCMC_J.XML",
        "LCMC_K.XML",
        "LCMC_L.XML",
        "LCMC_M.XML",
        "LCMC_N.XML",
        "LCMC_P.XML",
        "LCMC_R.XML"};

    enum LogLevel
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
}

#endif
