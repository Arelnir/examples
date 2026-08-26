// multi_resample.C
// 批量重采样：对每个位置的数据进行一次 60 keV 释放模拟，并绘制散点图
// 用法：root -l -q 'multi_resample.C("/mnt/sim/g4simbytsc/Analysis1", "posscan", 3000000)'
// 参数1：归档父目录；参数2：位置文件夹名前缀；参数3：采样次数（默认3000000）

#include <TFile.h>
#include <TTree.h>
#include <TRandom3.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

void multi_resample(const char* archiveParent = "/mnt/sim/g4simbytsc/Analysis1",
                    const char* prefix = "test",
                    Long64_t nSamples = 3000000) {
    // 有效电极区域（单位 mm）
    const double xLmin = -3.775, xLmax = -3.325;
    const double xRmin =  3.325, xRmax =  3.775;
    const double ymin = -0.52, ymax = 0.077665;

    // 获取所有位置文件夹
    TString cmd = TString::Format("ls -d %s/%s.*", archiveParent, prefix);
    TString dirList = gSystem->GetFromPipe(cmd);
    if (dirList.Length() == 0) {
        std::cerr << "No directories found matching pattern." << std::endl;
        return;
    }
    std::vector<std::string> dirs;
    std::stringstream ss(dirList.Data());
    std::string dir;
    while (std::getline(ss, dir)) {
        if (!dir.empty()) dirs.push_back(dir);
    }
    int nPositions = dirs.size();
    std::cout << "Found " << nPositions << " position directories." << std::endl;

    // 存储每个位置的左右沉积能量
    std::vector<double> leftValues, rightValues;

    // 随机数生成器（使用固定种子保证可重复；也可用0自动随机）
    TRandom3 rng(0);

    for (const auto& dir : dirs) {
        // 读取 primary 树获取所有事件 ID
        TFile *fprim = TFile::Open((dir + "/phonon_primary.root").c_str());
        if (!fprim || fprim->IsZombie()) {
            std::cerr << "Cannot open " << dir << "/phonon_primary.root" << std::endl;
            continue;
        }
        TTree *tprim = (TTree*)fprim->Get("primaryTree");
        int primEventID;
        tprim->SetBranchAddress("eventID", &primEventID);
        std::set<int> allEvents;
        Long64_t nPrim = tprim->GetEntries();
        for (Long64_t i=0; i<nPrim; ++i) {
            tprim->GetEntry(i);
            allEvents.insert(primEventID);
        }
        fprim->Close();

        // 读取 hits 树累加左右能量
        TFile *fhit = TFile::Open((dir + "/phonon_hits.root").c_str());
        if (!fhit || fhit->IsZombie()) {
            std::cerr << "Cannot open " << dir << "/phonon_hits.root" << std::endl;
            continue;
        }
        TTree *thit = (TTree*)fhit->Get("hitsTree");
        int hitEventID;
        double eDep, endX, endY;
        thit->SetBranchAddress("eventID", &hitEventID);
        thit->SetBranchAddress("eDep", &eDep);
        thit->SetBranchAddress("endX", &endX);
        thit->SetBranchAddress("endY", &endY);

        std::map<int, double> leftEnergy, rightEnergy;
        Long64_t nHits = thit->GetEntries();
        for (Long64_t i=0; i<nHits; ++i) {
            thit->GetEntry(i);
            bool isLeft  = (endX >= xLmin && endX <= xLmax && endY >= ymin && endY <= ymax);
            bool isRight = (endX >= xRmin && endX <= xRmax && endY >= ymin && endY <= ymax);
            if (!isLeft && !isRight) continue;
            if (isLeft)  leftEnergy[hitEventID] += eDep;
            if (isRight) rightEnergy[hitEventID] += eDep;
        }
        fhit->Close();

        // 构建事件向量
        std::vector<double> lVec, rVec;
        for (int ev : allEvents) {
            lVec.push_back(leftEnergy.count(ev) ? leftEnergy[ev] : 0.0);
            rVec.push_back(rightEnergy.count(ev) ? rightEnergy[ev] : 0.0);
        }
        if (lVec.empty()) {
            std::cerr << "No events found in " << dir << ", skipping." << std::endl;
            continue;
        }

        // 重采样求和
        Long64_t nEvents = lVec.size();
        double totalL = 0.0, totalR = 0.0;
        for (Long64_t i=0; i<nSamples; ++i) {
            Long64_t idx = rng.Integer(nEvents);
            totalL += lVec[idx];
            totalR += rVec[idx];
        }
        leftValues.push_back(totalL);
        rightValues.push_back(totalR);
        std::cout << dir << " : L=" << totalL << " eV, R=" << totalR << " eV" << std::endl;
    }

    // 保存文本结果
    std::ofstream outfile("resample_results.txt");
    if (outfile.is_open()) {
        outfile << "Left [eV]\tRight [eV]\n";
        for (size_t i=0; i<leftValues.size(); ++i) {
            outfile << leftValues[i] << "\t" << rightValues[i] << "\n";
        }
        outfile.close();
    }

    // 绘制散点图
    if (leftValues.empty()) {
        std::cerr << "No valid positions processed." << std::endl;
        return;
    }
    TGraph *gr = new TGraph(leftValues.size(), leftValues.data(), rightValues.data());
    gr->SetTitle("60 keV Release at Different Positions;Left Electrode Energy [eV];Right Electrode Energy [eV]");
    gr->SetMarkerStyle(20);
    gr->SetMarkerSize(1.0);
    gr->SetMarkerColor(kBlue);

    TCanvas *c = new TCanvas("c", "Multi Resample", 800, 600);
    c->SetLeftMargin(0.15);
    c->SetRightMargin(0.05);
    c->SetBottomMargin(0.15);
    gr->Draw("AP");
    c->SaveAs("multi_resample_scatter.png");

    // 保存图形到 root 文件
    TFile *fout = new TFile("multi_resample.root", "RECREATE");
    gr->Write("multi_scatter");
    fout->Close();

    std::cout << "Done. Results saved to resample_results.txt, multi_resample_scatter.png, multi_resample.root" << std::endl;
}