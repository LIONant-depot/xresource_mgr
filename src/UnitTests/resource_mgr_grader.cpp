#include "../xresource_mgr.h"
#include "TestGuids.h"
#include "TestResourceMgr.h"

int main(void)
{
    float TotalScore = 0;
    TotalScore += xresource::unitest::guids::Evaluate() * 0.25f;
    TotalScore += xresource::unitest::resource_type_registration::Evaluate() * 0.75f;

    printf( "\n\nFinal Score = %3.0f%%", TotalScore * 100 );

    return 0;
}