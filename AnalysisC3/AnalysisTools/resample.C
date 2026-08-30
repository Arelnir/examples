// resample.C
// 功能：从已有单个位置的模拟数据中，通过重采样构建一个 60 keV 释放的总沉积能量点。
// 用法：root -l -q 'resample.C(3000000)'   // 参数为采样次数，默认 3000000
// 依赖：当前目录需包含 phonon_primary.root 和 phonon_hits.root

#include <TFile.h>
#include <TTree.h>
#include <TRandom3.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>

void resample(Long64_t nSamples = 3000000) {
    // ========== 有效电极区域（单位 mm） ==========
    const double xLmin = -3.775, xLmax = -3.325;
    const double xRmin =  3.325, xRmax =  3.775;
    const double ymin = -0.52, ymax = 0.077665;

    // ========== 读取初级声子事件列表 ==========
    TFile *fprim = new TFile("phonon_primary.root");
    if (!fprim->IsOpen()) {
        std::cerr << "Error: cannot open phonon_primary.root" << std::endl;
        return;
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
    std::cout << "Total primary events: " << allEvents.size() << std::endl;

    // ========== 按事件累加左右电极沉积能量 ==========
    TFile *fhit = new TFile("phonon_hits.root");
    if (!fhit->IsOpen()) {
        std::cerr << "Error: cannot open phonon_hits.root" << std::endl;
        return;
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

    // 构建每个事件的 (L,R) 数组（包含所有初级事件，无击中的事件记 L=R=0）
    std::vector<double> leftVec, rightVec;
    for (int ev : allEvents) {
        leftVec.push_back(leftEnergy.count(ev) ? leftEnergy[ev] : 0.0);
        rightVec.push_back(rightEnergy.count(ev) ? rightEnergy[ev] : 0.0);
    }
    std::cout << "Number of event samples for resampling: " << leftVec.size() << std::endl;

    // ========== 随机采样并求和 ==========
    TRandom3 rng(0);   // 可以修改种子，0 表示使用系统时间
    double totalL = 0.0, totalR = 0.0;
    Long64_t nEvents = leftVec.size();
    for (Long64_t i=0; i<nSamples; ++i) {
        Long64_t idx = rng.Integer(nEvents);   // 随机选择一个事件
        totalL += leftVec[idx];
        totalR += rightVec[idx];
    }

    // 输出结果
    std::cout << "\n===== Resampling Result =====" << std::endl;
    std::cout << "Number of sampled phonons (events): " << nSamples << std::endl;
    std::cout << "Total left electrode deposited energy: " << totalL << " eV" << std::endl;
    std::cout << "Total right electrode deposited energy: " << totalR << " eV" << std::endl;
    std::cout << "Total deposited energy: " << totalL + totalR << " eV" << std::endl;
    std::cout << "Expected total deposited (based on avg per event): "
              << (totalL + totalR) / nSamples * nSamples << " eV" << std::endl; // 仅供参考
}