// check_energy_balance.C
// 用法：root -l -q check_energy_balance.C
// 输出：energy_balance.png

#include <TFile.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TStyle.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

void check_energy_balance() {
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kViridis);

    // ========== 读取初级声子数据 ==========
    double totalPrimaryEnergy = 0.0;
    std::vector<double> primX, primY, primE;

    std::ifstream pfile("phonon_primary.txt");
    if (!pfile.is_open()) {
        std::cerr << "Error: cannot open phonon_primary.txt" << std::endl;
        return;
    }
    std::string line;
    std::getline(pfile, line); // 跳过标题
    while (std::getline(pfile, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (ss >> token) {   // 空格分隔
            tokens.push_back(token);
        }
        if (tokens.size() < 8) continue;
        try {
            // 索引 0:RunID, 1:EventID, 2:ParticleName, 3:Energy(eV),
            //       4:X(mm), 5:Y(mm), 6:Z(mm), 7:T(ns)
            double energy = std::stod(tokens[3]);
            double x = std::stod(tokens[4]);
            double y = std::stod(tokens[5]);
            totalPrimaryEnergy += energy;
            primX.push_back(x);
            primY.push_back(y);
            primE.push_back(energy);
        } catch (...) { continue; }
    }
    pfile.close();
    std::cout << "Primary events: " << primX.size()
              << ", total energy: " << totalPrimaryEnergy << " eV" << std::endl;

    // ========== 读取击中数据 ==========
    double totalHitsEnergy = 0.0;
    std::ifstream hfile("phonon_hits.txt");
    if (!hfile.is_open()) {
        std::cerr << "Error: cannot open phonon_hits.txt" << std::endl;
        return;
    }
    std::getline(hfile, line); // 跳过标题
    while (std::getline(hfile, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }
        if (tokens.size() < 4) continue;
        try {
            // 索引 0:RunID, 1:EventID, 2:TrackID, 3:EnergyDeposited[eV]
            double eDep = std::stod(tokens[3]);
            totalHitsEnergy += eDep;
        } catch (...) { continue; }
    }
    hfile.close();
    std::cout << "Total hits energy: " << totalHitsEnergy << " eV" << std::endl;

    // ========== 读取低能声子数据 ==========
    double totalLowEnergy = 0.0;
    std::ifstream lfile("phonon_lowenergy.txt");
    if (lfile.is_open()) {
        std::getline(lfile, line); // 跳过标题
        while (std::getline(lfile, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }
            if (tokens.size() < 4) continue;
            try {
                double e = std::stod(tokens[3]);
                totalLowEnergy += e;
            } catch (...) { continue; }
        }
        lfile.close();
    } else {
        std::cout << "Warning: phonon_lowenergy.txt not found, setting lowE = 0" << std::endl;
    }
    std::cout << "Total low-energy phonon energy: " << totalLowEnergy << " eV" << std::endl;

    // 计算差值
    double deltaE = totalPrimaryEnergy - totalHitsEnergy - totalLowEnergy;
    double deltaFrac = (totalPrimaryEnergy > 0) ? 100.0 * deltaE / totalPrimaryEnergy : 0.0;

    // ========== 创建初始声子能量 XY 分布图 ==========
    TH2F *h_prim_xy = new TH2F("h_prim_xy",
                                "Primary Phonon Energy (XY);X [mm];Y [mm];Energy [eV]",
                                100, -5, 5, 100, -2, 2);   // 范围覆盖你的硅片
    for (size_t i = 0; i < primX.size(); ++i) {
        h_prim_xy->Fill(primX[i], primY[i], primE[i]);
    }

    // ========== 绘制 ==========
    TCanvas *c = new TCanvas("c", "Energy Balance", 1200, 600);
    c->Divide(2, 1);

    // 左图：初始声子能量 XY
    c->cd(1);
    h_prim_xy->Draw("COLZ");

    // 右图：文字信息
    c->cd(2);
    TPaveText *pt = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    pt->SetFillColor(0);
    pt->SetBorderSize(0);
    pt->SetTextSize(0.05);
    pt->SetTextAlign(12);

    pt->AddText("Energy Balance Summary");
    pt->AddText(" ");
    pt->AddText(Form("Total Primary Energy: %.4f eV", totalPrimaryEnergy));
    pt->AddText(Form("Total Hits Energy:    %.4f eV", totalHitsEnergy));
    pt->AddText(Form("Total Low-E Energy:   %.4f eV", totalLowEnergy));
    pt->AddText(Form("#DeltaE = E_{prim} - E_{hits} - E_{low}: %.4f eV", deltaE));
    pt->AddText(Form("#DeltaE / E_{prim}: %.2f %%", deltaFrac));
    pt->Draw();

    c->SaveAs("energy_balance.png");
    std::cout << "\nDone. Output: energy_balance.png" << std::endl;
}
