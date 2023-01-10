
// task3_2Dlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "pcap.h"

#pragma comment(lib,"Wpcap.lib")
#pragma comment(lib,"Ws2_32.lib")

// Ctask3_2Dlg 对话框
class Ctask3_2Dlg : public CDialogEx
{
// 构造
public:
	Ctask3_2Dlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TASK3_2_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	pcap_if_t* m_alldevs;//指向设备列表首部的指针	
	pcap_if_t* m_selectdevs;//当前选择的设备列表的指针	
	
	//afx_msg void OnLbnSelchangeListDev();
	//afx_msg void OnIpnFieldchangedIpaddressMask(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonBack();
	afx_msg void OnDestroy();
	void OnTimer(UINT nIDEvent);
	CListBox m_dev;
	CListBox m_log;
	CListBox m_routeTable;
	CIPAddressCtrl m_mask;
	CIPAddressCtrl m_dest;
	CIPAddressCtrl m_next;
};
