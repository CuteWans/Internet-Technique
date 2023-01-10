
// task3_2Dlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "pcap.h"

#pragma comment(lib,"Wpcap.lib")
#pragma comment(lib,"Ws2_32.lib")

// Ctask3_2Dlg �Ի���
class Ctask3_2Dlg : public CDialogEx
{
// ����
public:
	Ctask3_2Dlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_TASK3_2_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	pcap_if_t* m_alldevs;//ָ���豸�б��ײ���ָ��	
	pcap_if_t* m_selectdevs;//��ǰѡ����豸�б��ָ��	
	
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
