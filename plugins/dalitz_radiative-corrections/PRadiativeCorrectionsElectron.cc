#include "PRadiativeCorrectionsElectron.h"

using namespace std;


ClassImp(PRadiativeCorrectionsElectron)

PRadiativeCorrectionsElectron::PRadiativeCorrectionsElectron(const Char_t *id, const Char_t *de, Int_t key) :
    PRadiativeCorrections(id, de, key) {

    pi0 = eta = etap = false;

    corrections_eta = new TGraph2D("eta_ee_corrections", "Radiative Corrections #eta #to e^{+} e^{-} #gamma",
                                   eta_ee::corr.size(), &eta_ee::x[0], &eta_ee::y[0], &eta_ee::corr[0]);
    corrections_etap = new TGraph2D("etap_ee_corrections", "Radiative Corrections #eta' #to e^{+} e^{-} #gamma",
                                    etap_ee::corr.size(), &etap_ee::x[0], &etap_ee::y[0], &etap_ee::corr[0]);

    // calling GetHistogram() method avoids Interpolate() returning 0 if an exact point is being interpolated
    h_corrections_eta = corrections_eta->GetHistogram();
    h_corrections_etap = corrections_etap->GetHistogram();
}

PDistribution* PRadiativeCorrectionsElectron::Clone(const char*delme) const {
    return new PRadiativeCorrectionsElectron((const PRadiativeCorrectionsElectron &)* this);
}

void PRadiativeCorrectionsElectron::SetLimits()
{
    TGraph2D* corr;

    if (parent->GetParent()->Is("eta"))
        corr = corrections_eta;
    else if (parent->GetParent()->Is("eta'"))
        corr = corrections_etap;

    weight_max += corr->GetZmax()/100.;
    x_min = corr->GetXmin();

    limits_set = true;
}

Double_t PRadiativeCorrectionsElectron::GetWeight()
{
    if (!limits_set)
        SetLimits();

    meson = parent->GetParent();
    pi0 = eta = etap = false;
    if (!meson->IsMeson()) {
        Warning("GetWeight","Grandparent is not a meson");
        return 1.;
    }
    if (meson->Is("pi0"))
        pi0 = true;
    else if (meson->Is("eta"))
        eta = true;
    else if (meson->Is("eta'"))
        etap = true;
    else {
        Warning("GetWeight","No pseudo-scalar meson found");
        return 1.;
    }

    double x = parent->M2()/meson->M2();
    double y = meson->Vect4().Dot(lp->Vect4()-lm->Vect4());
    // use absolute value for y since correction values are just provided for positive y values
    // y should be symmetric so under this assumption everything should be fine
    y = 2*abs(y)/meson->M2()/(1-x);

    // check if the current x value is smaller than the minimum provided in the correction tables
    // if so, use the minimum x value instead to use this value instead
    if (x < x_min)
        x = x_min;

    double weight = 1.;
    double correction = 0.;

    if (pi0)
        correction = corrections_pi0->Interpolate(x, y);
    else if (eta)
        correction = h_corrections_eta->Interpolate(x, y);
    else if (etap)
        correction = h_corrections_etap->Interpolate(x, y);
    else
        Warning("GetWeight","No correction table found for this channel");

    weight += correction/100.;

    return weight;
}