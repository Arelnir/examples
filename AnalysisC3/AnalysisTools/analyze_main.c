// analyze_main.C
// 用法：在包含合并后的 phonon_primary.root 和 phonon_hits.root 的目录中执行
// root -l -q analyze_main.C
// 输出：primary_xy.png, time_deposition.png, energy_spectrum.png,
//       incident_vs_deposited.png, scatter_left_right.png, analysis_output.root

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TH1F.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TPad.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>

void analyze_main() {
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kViridis);

    // ========== 读取初级声子数据 ==========
    TFile *fprim = new TFile("phonon_primary.root");
    if (!fprim->IsOpen()) { std::cerr << "Error opening phonon_primary.root\n"; return; }
    TTree *tprim = (TTree*)fprim->Get("primaryTree");
    int primEventID;
    double primEnergy, primX, primY, primT;
    tprim->SetBranchAddress("eventID", &primEventID);
    tprim->SetBranchAddress("energy", &primEnergy);
    tprim->SetBranchAddress("x", &primX);
    tprim->SetBranchAddress("y", &primY);
    tprim->SetBranchAddress("t", &primT);

    std::map<int, double> primTimeMap;    // eventID -> primary time (ns)
    std::set<int> primaryEvents;
    std::vector<double> vX, vY, vE;

    Long64_t nPrim = tprim->GetEntries();
    for (Long64_t i=0; i<nPrim; ++i) {
        tprim->GetEntry(i);
        primTimeMap[primEventID] = primT;
        primaryEvents.insert(primEventID);
        vX.push_back(primX);
        vY.push_back(primY);
        vE.push_back(primEnergy);
    }
    std::cout << "Primary events: " << nPrim << std::endl;

    // ========== 读取击中数据 ==========
    TFile *fhit = new TFile("phonon_hits.root");
    if (!fhit->IsOpen()) { std::cerr << "Error opening phonon_hits.root\n"; return; }
    TTree *thit = (TTree*)fhit->Get("hitsTree");
    int hitEventID;
    double eDep, startEnergy, endX, endY, hitEndTime;
    thit->SetBranchAddress("eventID", &hitEventID);
    thit->SetBranchAddress("eDep", &eDep);
    thit->SetBranchAddress("startEnergy", &startEnergy);   // 入射能量
    thit->SetBranchAddress("endX", &endX);
    thit->SetBranchAddress("endY", &endY);
    thit->SetBranchAddress("endTime", &hitEndTime);

    const double xLmin = -3.775, xLmax = -3.325;
    const double xRmin =  3.325, xRmax =  3.775;
    const double ymin = -0.52, ymax = 0.077665;

    std::map<int, double> leftEnergy, rightEnergy;

    // 时间-沉积能量谱：时间范围 0–1 ms（1e6 ns）
    TH1F *h_time = new TH1F("h_time",
                            "Deposited Energy vs Time from Primary;Time [ns];Total Energy [eV]",
                            200, 0, 1e6);

    // 入射能量谱（计数）
    TH1F *h_inc_energy = new TH1F("h_inc_energy",
                                  "Incident Energy Spectrum;Incident Energy [eV];Number of Hits",
                                  200, 0, 0.035);

    // 入射能量 vs 沉积能量 二维直方图
    TH2F *h_inc_vs_dep = new TH2F("h_inc_vs_dep",
                                  "Incident vs Deposited Energy;Incident Energy [eV];Deposited Energy [eV];Counts",
                                  200, 0, 0.035,
                                  200, 0, 0.035);

    Long64_t nHits = thit->GetEntries();
    for (Long64_t i=0; i<nHits; ++i) {
        thit->GetEntry(i);

        bool isLeft  = (endX >= xLmin && endX <= xLmax && endY >= ymin && endY <= ymax);
        bool isRight = (endX >= xRmin && endX <= xRmax && endY >= ymin && endY <= ymax);
        if (!isLeft && !isRight) continue;

        if (isLeft)  leftEnergy[hitEventID] += eDep;
        if (isRight) rightEnergy[hitEventID] += eDep;

        double t0 = primTimeMap.count(hitEventID) ? primTimeMap[hitEventID] : 0.0;
        double totalTime = hitEndTime - t0;
        h_time->Fill(totalTime, eDep);

        h_inc_energy->Fill(startEnergy);
        h_inc_vs_dep->Fill(startEnergy, eDep);
    }
    std::cout << "Total hits: " << nHits << std::endl;

    // ========== 初始XY能量分布图 ==========
    TH2F *h_prim_xy = new TH2F("h_prim_xy",
                               "Primary Phonon Energy (XY);X [mm];Y [mm];Energy [eV]",
                               100, -5.0, 5.0, 100, -1.667, 1.667);
    for (size_t i=0; i<vX.size(); ++i) h_prim_xy->Fill(vX[i], vY[i], vE[i]);

    // ========== 左右能量沉积密度图 ==========
    TH2F *h_scatter = new TH2F("h_scatter",
                               "Left vs Right Electrode Deposited Energy;Left Energy [eV];Right Energy [eV];Counts",
                               200, 0, 0.035,
                               200, 0, 0.035);

    for (int ev : primaryEvents) {
        double l = leftEnergy.count(ev) ? leftEnergy[ev] : 0.0;
        double r = rightEnergy.count(ev) ? rightEnergy[ev] : 0.0;
        h_scatter->Fill(l, r);
    }

    // ========== 绘图设置：增加四周边距 ==========
    auto setMargins = [](TCanvas* c) {
        c->SetLeftMargin(0.18);
        c->SetRightMargin(0.18);
        c->SetBottomMargin(0.18);
        c->SetTopMargin(0.12);
    };

    // ========== 保存各图 ==========
    TCanvas *c1 = new TCanvas("c1", "Primary XY", 800, 600);
    setMargins(c1);
    h_prim_xy->Draw("COLZ");
    c1->SaveAs("primary_xy.png");

    TCanvas *c2 = new TCanvas("c2", "Time", 800, 600);
    setMargins(c2);
    h_time->Draw("HIST");
    c2->SaveAs("time_deposition.png");

    TCanvas *c3 = new TCanvas("c3", "Incident Energy", 800, 600);
    setMargins(c3);
    h_inc_energy->Draw("HIST");
    c3->SaveAs("energy_spectrum.png");

    TCanvas *c4 = new TCanvas("c4", "Incident vs Deposited", 800, 600);
    setMargins(c4);
    h_inc_vs_dep->Draw("COLZ");
    c4->SaveAs("incident_vs_deposited.png");

    TCanvas *c5 = new TCanvas("c5", "Scatter Density", 800, 600);
    setMargins(c5);
    h_scatter->Draw("COLZ");
    c5->SaveAs("scatter_left_right.png");

    // 保存直方图到 ROOT 文件
    TFile *out = new TFile("analysis_output.root", "RECREATE");
    h_prim_xy->Write();
    h_time->Write();
    h_inc_energy->Write();
    h_inc_vs_dep->Write();
    h_scatter->Write();
    out->Close();

    std::cout << "Analysis complete. Output: primary_xy.png, time_deposition.png, "
              << "energy_spectrum.png, incident_vs_deposited.png, scatter_left_right.png, "
              << "analysis_output.root" << std::endl;
}