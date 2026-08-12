// check_energy_balance_root.C
// 用法：root -l -q check_energy_balance_root.C
// 输出：energy_balance.png

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TPie.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TPad.h>
#include <iostream>
#include <vector>

void check_energy_balance_root() {
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kViridis);

    // ========== 读取初级声子数据 ==========
    TFile *fprim = new TFile("phonon_primary.root");
    if (!fprim->IsOpen()) { std::cerr << "Error opening phonon_primary.root\n"; return; }
    TTree *tprim = (TTree*)fprim->Get("primaryTree");
    double primEnergy, primX, primY;
    tprim->SetBranchAddress("energy", &primEnergy);
    tprim->SetBranchAddress("x", &primX);
    tprim->SetBranchAddress("y", &primY);

    double totalPrimaryEnergy = 0.0;
    std::vector<double> vX, vY, vE;
    Long64_t nEntries = tprim->GetEntries();
    for (Long64_t i=0; i<nEntries; ++i) {
        tprim->GetEntry(i);
        totalPrimaryEnergy += primEnergy;
        vX.push_back(primX);
        vY.push_back(primY);
        vE.push_back(primEnergy);
    }
    std::cout << "Primary events: " << nEntries
              << ", total energy: " << totalPrimaryEnergy << " eV" << std::endl;

    // ========== 读取有效电极击中 ==========
    TFile *fact = new TFile("phonon_hits_active.root");
    if (!fact->IsOpen()) { std::cerr << "Error opening phonon_hits_active.root\n"; return; }
    TTree *tact = (TTree*)fact->Get("hitsTree");
    double activeEDep;
    tact->SetBranchAddress("eDep", &activeEDep);
    double totalActiveEnergy = 0.0;
    for (Long64_t i=0; i<tact->GetEntries(); ++i) {
        tact->GetEntry(i);
        totalActiveEnergy += activeEDep;
    }

    // ========== 读取无效电极击中 ==========
    TFile *fpas = new TFile("phonon_hits_passive.root");
    if (!fpas->IsOpen()) { std::cerr << "Error opening phonon_hits_passive.root\n"; return; }
    TTree *tpas = (TTree*)fpas->Get("hitsTree");
    double passiveEDep;
    tpas->SetBranchAddress("eDep", &passiveEDep);
    double totalPassiveEnergy = 0.0;
    for (Long64_t i=0; i<tpas->GetEntries(); ++i) {
        tpas->GetEntry(i);
        totalPassiveEnergy += passiveEDep;
    }

    // ========== 读取低能声子数据 ==========
    TFile *flow = new TFile("phonon_lowenergy.root");
    double totalLowEnergy = 0.0;
    if (flow->IsOpen()) {
        TTree *tlow = (TTree*)flow->Get("lowETree");
        double lowEnergy;
        tlow->SetBranchAddress("energy", &lowEnergy);
        for (Long64_t i=0; i<tlow->GetEntries(); ++i) {
            tlow->GetEntry(i);
            totalLowEnergy += lowEnergy;
        }
    } else {
        std::cout << "Warning: phonon_lowenergy.root not found, setting lowE = 0" << std::endl;
    }

    double deltaE = totalPrimaryEnergy - totalActiveEnergy - totalPassiveEnergy - totalLowEnergy;
    double deltaFrac = (totalPrimaryEnergy > 0) ? 100.0 * deltaE / totalPrimaryEnergy : 0.0;

    // ========== 创建初始声子能量 XY 分布图 ==========
    TH2F *h_prim_xy = new TH2F("h_prim_xy",
                               "Primary Phonon Energy (XY);X [mm];Y [mm];Energy [eV]",
                               100, -5.0, 5.0, 60, -1.667, 1.667);  // 60 bins 使 bin 宽接近 100um
    for (size_t i=0; i<vX.size(); ++i) h_prim_xy->Fill(vX[i], vY[i], vE[i]);

    // ========== 绘图 ==========
    TCanvas *c = new TCanvas("c", "Energy Balance", 1500, 600);
    c->Divide(2,1);

    // 左图：初级能量 XY，等比例尺
    c->cd(1);
    gPad->SetLeftMargin(0.15);
    gPad->SetBottomMargin(0.12);
    h_prim_xy->Draw("COLZ");
    // 设置等比例尺（X和Y轴的坐标跨度与实际长度比例一致）
    gPad->SetFixedAspectRatio();
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.035);
    latex.SetTextColor(kWhite);
    latex.DrawLatex(0.15, 0.85, Form("Total Primary: %.2f eV", totalPrimaryEnergy));

    // 右图：文字信息 + 扇形图（叠加）
    c->cd(2);
    TPaveText *pt = new TPaveText(0.05, 0.65, 0.95, 0.95, "NDC");
    pt->SetFillColor(0);
    pt->SetBorderSize(0);
    pt->SetTextSize(0.04);
    pt->SetTextAlign(12);
    pt->AddText("Energy Balance Summary");
    pt->AddText(Form("Primary:  %.4f eV", totalPrimaryEnergy));
    pt->AddText(Form("Active:   %.4f eV (%.1f%%)", totalActiveEnergy,
                     100.*totalActiveEnergy/totalPrimaryEnergy));
    pt->AddText(Form("Passive:  %.4f eV (%.1f%%)", totalPassiveEnergy,
                     100.*totalPassiveEnergy/totalPrimaryEnergy));
    pt->AddText(Form("Low-E:    %.4f eV (%.1f%%)", totalLowEnergy,
                     100.*totalLowEnergy/totalPrimaryEnergy));
    pt->AddText(Form("#DeltaE = %.4f eV (%.2f%%)", deltaE, deltaFrac));
    pt->Draw();

    // 扇形图
    TCanvas *c2 = new TCanvas("c2", "Pie", 400, 400);
    TH1F *h_pie = new TH1F("h_pie", "Energy Partition", 3, 0, 3);
    h_pie->SetBinContent(1, totalActiveEnergy);
    h_pie->SetBinContent(2, totalPassiveEnergy);
    h_pie->SetBinContent(3, totalLowEnergy);
    h_pie->GetXaxis()->SetBinLabel(1, Form("Active (%.1f%%)", 100.*totalActiveEnergy/totalPrimaryEnergy));
    h_pie->GetXaxis()->SetBinLabel(2, Form("Passive (%.1f%%)", 100.*totalPassiveEnergy/totalPrimaryEnergy));
    h_pie->GetXaxis()->SetBinLabel(3, Form("Low-E (%.1f%%)", 100.*totalLowEnergy/totalPrimaryEnergy));
    TPie *pie = new TPie(h_pie);
    pie->SetTextSize(0.06);
    pie->Draw();
    c2->SaveAs("energy_pie.png");

    c->SaveAs("energy_balance.png");
    std::cout << "\nDone. Output: energy_balance.png and energy_pie.png" << std::endl;
}