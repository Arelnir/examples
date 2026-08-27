// check_energy_balance_root.C
// 用法：root -l -q check_energy_balance_root.C
// 输出：energy_balance.png, hits_xy_xz.png

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TH1F.h>
#include <TPie.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TPad.h>
#include <TColor.h>
#include <iostream>
#include <vector>

void check_energy_balance_root() {
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kViridis);

    // ★★★ 唯一需要修改的路径变量 ★★★
    const char* dataPath = "/mnt/sim/g4simbytsc/temp/";

    // ========== 读取初级声子数据 ==========
    TFile *fprim = new TFile(Form("%s/phonon_primary.root", dataPath));
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
    TFile *fact = new TFile(Form("%s/phonon_hits_active.root", dataPath));
    if (!fact->IsOpen()) { std::cerr << "Error opening phonon_hits_active.root\n"; return; }
    TTree *tact = (TTree*)fact->Get("hitsTree");
    double activeEDep;
    tact->SetBranchAddress("eDep", &activeEDep);
    double totalActiveEnergy = 0.0;
    for (Long64_t i=0; i<tact->GetEntries(); ++i) {
        tact->GetEntry(i);
        totalActiveEnergy += activeEDep;
    }
    std::cout << "Active electrode energy: " << totalActiveEnergy << " eV" << std::endl;

    // ========== 读取无效电极击中 ==========
    TFile *fpas = new TFile(Form("%s/phonon_hits_passive.root", dataPath));
    if (!fpas->IsOpen()) { std::cerr << "Error opening phonon_hits_passive.root\n"; return; }
    TTree *tpas = (TTree*)fpas->Get("hitsTree");
    double passiveEDep;
    tpas->SetBranchAddress("eDep", &passiveEDep);
    double totalPassiveEnergy = 0.0;
    for (Long64_t i=0; i<tpas->GetEntries(); ++i) {
        tpas->GetEntry(i);
        totalPassiveEnergy += passiveEDep;
    }
    std::cout << "Passive electrode energy: " << totalPassiveEnergy << " eV" << std::endl;

    // ========== 读取低能声子数据 ==========
    TFile *flow = new TFile(Form("%s/phonon_lowenergy.root", dataPath));
    double totalLowEnergy = 0.0;
    if (flow->IsOpen()) {
        TTree *tlow = (TTree*)flow->Get("lowETree");
        if (tlow) {
            double lowEnergy;
            tlow->SetBranchAddress("energy", &lowEnergy);
            for (Long64_t i=0; i<tlow->GetEntries(); ++i) {
                tlow->GetEntry(i);
                totalLowEnergy += lowEnergy;
            }
            std::cout << "Low-energy phonons: " << tlow->GetEntries()
                      << " entries, total energy: " << totalLowEnergy << " eV" << std::endl;
        } else {
            std::cerr << "Warning: lowETree not found in phonon_lowenergy.root" << std::endl;
        }
    } else {
        std::cerr << "Warning: phonon_lowenergy.root not found, lowE = 0" << std::endl;
    }

    double deltaE = totalPrimaryEnergy - totalActiveEnergy - totalPassiveEnergy - totalLowEnergy;
    double deltaFrac = (totalPrimaryEnergy > 0) ? 100.0 * deltaE / totalPrimaryEnergy : 0.0;

    // ========== 创建直方图（格子正方形：100 bins × 100 bins）==========
    TH2F *h_prim_xy = new TH2F("h_prim_xy",
                               "Primary Phonon Energy (XY);X [mm];Y [mm];Energy [eV]",
                               100, -5.0, 5.0,
                               100, -1.667, 1.667);
    for (size_t i=0; i<vX.size(); ++i) h_prim_xy->Fill(vX[i], vY[i], vE[i]);

    // ========== 扇形图 ==========
    TH1F *h_pie = new TH1F("h_pie", "Energy Partition", 3, 0, 3);
    h_pie->SetBinContent(1, totalActiveEnergy);
    h_pie->SetBinContent(2, totalPassiveEnergy);
    h_pie->SetBinContent(3, totalLowEnergy);
    h_pie->GetXaxis()->SetBinLabel(1, Form("Active (%.1f%%)", 100.*totalActiveEnergy/totalPrimaryEnergy));
    h_pie->GetXaxis()->SetBinLabel(2, Form("Passive (%.1f%%)", 100.*totalPassiveEnergy/totalPrimaryEnergy));
    h_pie->GetXaxis()->SetBinLabel(3, Form("Low-E (%.1f%%)", 100.*totalLowEnergy/totalPrimaryEnergy));
    TPie *pie = new TPie(h_pie);
    pie->SetEntryFillColor(0, kBlue-9);
    pie->SetEntryFillColor(1, kGreen-9);
    pie->SetEntryFillColor(2, kYellow-7);
    pie->SetTextSize(0.035);

    // ========== 绘制能量平衡大图 ==========
    TCanvas *cAll = new TCanvas("cAll", "Energy Balance", 1400, 600);
    cAll->Divide(2,1);

    // ---- 左图：初级能量 XY (强制正方形) ----
    cAll->cd(1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.18);
    gPad->SetBottomMargin(0.15);
    h_prim_xy->Draw("COLZ");
    h_prim_xy->GetXaxis()->SetTitleSize(0.045);
    h_prim_xy->GetYaxis()->SetTitleSize(0.045);
    h_prim_xy->GetXaxis()->SetLabelSize(0.04);
    h_prim_xy->GetYaxis()->SetLabelSize(0.04);
    gPad->SetFixedAspectRatio();

    // ---- 右图：上扇形图，下文字 ----
    cAll->cd(2);
    TPad *padPie = new TPad("padPie", "Pie", 0.0, 0.4, 1.0, 1.0);
    padPie->SetLeftMargin(0.1);
    padPie->SetRightMargin(0.2);
    padPie->Draw();
    padPie->cd();
    pie->SetTextSize(0.035);
    pie->Draw("3d");

    cAll->cd(2);
    TPad *padText = new TPad("padText", "Text", 0.0, 0.0, 1.0, 0.4);
    padText->SetFillColor(0);
    padText->Draw();
    padText->cd();
    TPaveText *pt = new TPaveText(0.05, 0.1, 0.95, 0.9, "NDC");
    pt->SetFillColor(0);
    pt->SetBorderSize(0);
    pt->SetTextSize(0.08);
    pt->SetTextAlign(12);
    pt->AddText("Energy Balance Summary");
    pt->AddText(Form("Primary: %.4f eV", totalPrimaryEnergy));
    pt->AddText(Form("Active:  %.4f eV", totalActiveEnergy));
    pt->AddText(Form("Passive: %.4f eV", totalPassiveEnergy));
    pt->AddText(Form("Low-E:   %.4f eV", totalLowEnergy));
    pt->AddText(Form("#DeltaE = %.4f eV (%.2f%%)", deltaE, deltaFrac));
    pt->Draw();

    cAll->SaveAs(Form("%s/energy_balance.png", dataPath));

    // ========== 新增：所有 hits 声子的 XY 和 XZ 能量分布 ==========
    // 合并有效与无效击中数据
    TH2F *h_xy_all = new TH2F("h_xy_all",
                             "All Hits Energy (XY);X [mm];Y [mm];Energy [eV]",
                             200, -5.0, 5.0, 100, -1.667, 1.667);
    TH2F *h_xz_all = new TH2F("h_xz_all",
                             "All Hits Energy (XZ);X [mm];Z [mm];Energy [eV]",
                             200, -5.0, 5.0, 100, -0.5, 0.5);

    auto fill_hits_from_file = [&](TFile *file) {
        if (!file || !file->IsOpen()) return;
        TTree *tree = (TTree*)file->Get("hitsTree");
        if (!tree) return;
        double endX, endY, endZ, eDep;
        tree->SetBranchAddress("endX", &endX);
        tree->SetBranchAddress("endY", &endY);
        tree->SetBranchAddress("endZ", &endZ);
        tree->SetBranchAddress("eDep", &eDep);
        Long64_t n = tree->GetEntries();
        for (Long64_t i=0; i<n; ++i) {
            tree->GetEntry(i);
            h_xy_all->Fill(endX, endY, eDep);
            h_xz_all->Fill(endX, endZ, eDep);
        }
    };

    // 填充有效和无效
    fill_hits_from_file(fact);   // 有效
    fill_hits_from_file(fpas);   // 无效

    TCanvas *cHits = new TCanvas("cHits", "Hits XY/XZ", 1400, 600);
    cHits->Divide(2,1);

    cHits->cd(1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.15);
    h_xy_all->Draw("COLZ");

    cHits->cd(2);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.15);
    gPad->SetBottomMargin(0.15);
    h_xz_all->Draw("COLZ");

    cHits->SaveAs(Form("%s/hits_xy_xz.png", dataPath));

    std::cout << "\nDone. Output: energy_balance.png, hits_xy_xz.png" << std::endl;
}