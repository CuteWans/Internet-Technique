
// task3_2Dlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "task3_2.h"
#include "task3_2Dlg.h"
#include "pcap.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAX_IF 5 // ���ӿ���Ŀ

#pragma pack(1)
typedef struct FrameHeader_t { // ֡�ײ�
    UCHAR DesMAC[6]; // Ŀ�ĵ�ַ
    UCHAR SrcMAC[6]; // Դ��ַ
    USHORT FrameType; // ֡����
} FrameHeader_t;

typedef struct ARPFrame_t {	// ARP֡
	FrameHeader_t FrameHeader; // ֡�ײ�
    WORD HardwareType; // Ӳ������
	WORD ProtocolType; // Э������
	BYTE HLen; // Ӳ����ַ����
	BYTE PLen; // Э���ַ����
	WORD Operation; // ����ֵ
	UCHAR SendHa[6]; // ԴMAC��ַ
	ULONG SendIP; // ԴIP��ַ
	UCHAR RecvHa[6]; // Ŀ��MAC��ַ
	ULONG RecvIP; // Ŀ��IP��ַ
} ARPFrame_t;

typedef struct IPHeader_t {	// IP�ײ�
	BYTE Ver_HLen; // �汾+ͷ������
	BYTE TOS; // ��������
	WORD TotalLen; // �ܳ���
	WORD ID; // ��ʶ
	WORD Flag_Segment; // ��־+Ƭƫ��
	BYTE TTL; // ����ʱ��
	BYTE Protocol; // Э��
	WORD Checksum; // ͷ��У���
	ULONG SrcIP; // ԴIP��ַ
	ULONG DstIP; // Ŀ��IP��ַ
} IPHeader_t;

typedef struct ICMPHeader_t { // ICMP�ײ�
	BYTE Type; // ����
	BYTE Code; // ����
	WORD Checksum; // У���
	WORD Id; // ��ʶ
	WORD Sequence; // ���к�
} ICMPHeader_t;

typedef struct IPFrame_t { // IP֡
	FrameHeader_t FrameHeader; // ֡�ײ�
	IPHeader_t IPHeader; // IP�ײ�
} IPFrame_t;

//#pragma pack()

typedef struct ip_t { // �����ַ
	ULONG IPAddr; // IP��ַ
	ULONG IPMask; // ��������
} ip_t;

typedef struct  IfInfo_t { // �ӿ���Ϣ
	char* DeviceName; // �豸��
	CString Description; // �豸����
	UCHAR MACAddr[6]; // MAC��ַ
	CArray <ip_t,ip_t&>	ip; // IP��ַ�б�
	pcap_t *adhandle; // pcap���
} IfInfo_t;

typedef struct SendPacket_t { // �������ݰ��ṹ
	int	len; // ����
	BYTE PktData[2000]; // ���ݻ���
	ULONG TargetIP; // Ŀ��IP��ַ
	UINT_PTR n_mTimer; // ��ʱ��
	UINT IfNo; // �ӿ����
} SendPacket_t;

typedef struct RouteTable_t { // ·�ɱ���ṹ
	ULONG Mask; // ��������
	ULONG DstIP; // Ŀ�ĵ�ַ
	ULONG NextHop; // ��һ����
	UINT IfNo; // �ӿ����
} RouteTable_t;

typedef struct IP_MAC_t { // IP-MAC��ַӳ��ṹ
	ULONG IPAddr; // IP��ַ
	UCHAR MACAddr[6]; // MAC��ַ
} IP_MAC_t;

// -----------------ȫ�ֱ���------------------
IfInfo_t IfInfo[MAX_IF]; // �ӿ���Ϣ����
int	IfCount; // �ӿڸ���
UINT_PTR TimerCount; // ��ʱ������

CList<SendPacket_t, SendPacket_t&> SP; // �������ݰ��������
CList<IP_MAC_t, IP_MAC_t&> IP_MAC; // IP-MAC��ַӳ���б�
CList<RouteTable_t, RouteTable_t&> RouteTable; // ·�ɱ�

Ctask3_2Dlg	*pDlg; // �Ի���ָ��

CMutex mMutex(0,0,0); //����
//CMutex( 
//BOOL bInitiallyOwn /* = FALSE */,    //����ָ������������ʼ״̬������(TRUE)���Ƿ�����(FALSE) 
//LPCTSTR lpszName /* = NULL */,        //����ָ������������� 
//LPSECURITY_ATTRIBUTES lpsaAttribute /* = NULL */        //Ϊһ��ָ��SECURITY_ATTRIBUTES�ṹ��ָ�� 

// -------------------ȫ�ֺ���--------------------
// IP��ַת��
CString IPntoa(ULONG nIPAddr);

// MAC��ַת��
CString MACntoa(UCHAR *nMACAddr);

// MAC��ַ�Ƚ�
bool cmpMAC(UCHAR *MAC1, UCHAR *MAC2);

// MAC��ַ����
void cpyMAC(UCHAR *MAC1, UCHAR *MAC2);

// MAC��ַ����
void setMAC(UCHAR *MAC, UCHAR ch);

// IP��ַ��ѯ
bool IPLookup(ULONG ipaddr, UCHAR *p);

// ���ݰ������߳�
UINT Capture(PVOID pParam);

// ��ȡ���ؽӿ�MAC��ַ�߳�
UINT CaptureLocalARP(PVOID pParam);

// ����ARP����
void ARPRequest(pcap_t *adhandle, UCHAR *srcMAC, ULONG srcIP, ULONG targetIP);

// ��ѯ·�ɱ�
DWORD RouteLookup(UINT &ifNO, DWORD desIP, CList <RouteTable_t, RouteTable_t&> routeTable);

// ����ARP���ݰ�
void ARPPacketProc(struct pcap_pkthdr *header, const u_char *pkt_data);

// ����IP���ݰ�
void IPPacketProc(IfInfo_t *pIfInfo, struct pcap_pkthdr *header, const u_char *pkt_data);

// ����ICMP���ݰ�
void ICMPPacketProc(IfInfo_t *pIfInfo, BYTE type, BYTE code, const u_char *pkt_data);

// ���IP���ݰ�ͷ��У����Ƿ���ȷ
int IsChecksumRight(char * buffer);

// ����У���
unsigned short ChecksumCompute(unsigned short *buffer, int size);

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();
	
// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Ctask3_2Dlg �Ի���




Ctask3_2Dlg::Ctask3_2Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Ctask3_2Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ctask3_2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DEV, m_dev);
	DDX_Control(pDX, IDC_LIST_LOG, m_log);
	DDX_Control(pDX, IDC_LIST_ROUTETABLE, m_routeTable);
	DDX_Control(pDX, IDC_IPADDRESS_MASK, m_mask);
	DDX_Control(pDX, IDC_IPADDRESS_DEST, m_dest);
	DDX_Control(pDX, IDC_IPADDRESS_NEXT, m_next);
}

BEGIN_MESSAGE_MAP(Ctask3_2Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &Ctask3_2Dlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &Ctask3_2Dlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, &Ctask3_2Dlg::OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_BACK, &Ctask3_2Dlg::OnBnClickedButtonBack)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// Ctask3_2Dlg ��Ϣ�������

BOOL Ctask3_2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	Ctask3_2App* pApp = (Ctask3_2App*)AfxGetApp();
	pDlg = (Ctask3_2Dlg*)pApp->m_pMainWnd;
	
	char errbuf[PCAP_ERRBUF_SIZE], strbuf[1000];

	// ��ñ������豸�б�
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL /*������֤*/, &m_alldevs, errbuf) == -1)
    {
		// ���󣬷��ش�����Ϣ
        sprintf_s(strbuf, "pcap_findalldevs_ex����: %s", errbuf);
		//MessageBox(strbuf);
		PostMessage(WM_QUIT, 0, 0);
    }

	for(m_selectdevs = m_alldevs; m_selectdevs != NULL; m_selectdevs= m_selectdevs->next) // ��ʾ�ӿ��б�
		m_dev.AddString(CString(m_selectdevs->name)); // ����d->name��ȡ������ӿ��豸������
	
	m_dev.SetCurSel(0);
	m_dev.SetHorizontalExtent(1600);
	m_log.SetHorizontalExtent(1600);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void Ctask3_2Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Ctask3_2Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR Ctask3_2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



//void Ctask3_2Dlg::OnLbnSelchangeListDev()
//{
//	// TODO: �ڴ���ӿؼ�֪ͨ����������
//}
//
//
//void Ctask3_2Dlg::OnIpnFieldchangedIpaddressMask(NMHDR *pNMHDR, LRESULT *pResult)
//{
//	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
//	// TODO: �ڴ���ӿؼ�֪ͨ����������
//	*pResult = 0;
//}


void Ctask3_2Dlg::OnBnClickedButtonStart()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);   
	m_dev.EnableWindow(FALSE);

	// ��ȡ�����Ľӿ��б�
	pcap_addr_t	*a;
	struct bpf_program fcode; // struct bpf_program:ʹ����pcap_compile����ʽ����
    char errbuf[PCAP_ERRBUF_SIZE], strbuf[1000];
	int i = 0, j = 0, k = 0;
	ip_t ipaddr;
	UCHAR srcMAC[6];
	ULONG srcIP;

	// ѡ��ӿ�
	int N = m_dev.GetCurSel(); // ��ȡlistbox��ѡ�е��е���Ŀ
	m_selectdevs = m_alldevs;
	while(N--)
		m_selectdevs = m_selectdevs->next;

	IfInfo[0].DeviceName = m_selectdevs->name;
	IfInfo[0].Description = m_selectdevs->description;
	for(a = m_selectdevs->addresses; a; a = a->next)	
	{
		if (a->addr->sa_family == AF_INET)
		{
           	ipaddr.IPAddr = (((struct sockaddr_in *)a->addr)->sin_addr.s_addr);
			ipaddr.IPMask = (((struct sockaddr_in *)a->netmask)->sin_addr.s_addr);
			IfInfo[0].ip.Add(ipaddr);
			j++;
		}
	}	

	// ������·����IP��ַ��ĿҪ��
	if (j < 2)
	{
		MessageBox(L"��·�ɳ���Ҫ�󱾵���������Ӧ����2��IP��ַ");
		GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);   
		m_dev.EnableWindow(TRUE);
		return;
	}

	// ����ʵ�ʵ�������
	IfCount = 1;//IfCount = i;	

	// �򿪽ӿ�
	for (i=0; i < IfCount; i++)
	{

		if ( (IfInfo[i].adhandle = pcap_open(IfInfo[i].DeviceName, // �豸��
			                       65536,                          // ��������
					               PCAP_OPENFLAG_PROMISCUOUS,      // ����ģʽ
						           1000,                           // ��ʱʱ��
							       NULL,                           // Զ����֤
								   errbuf                          // ���󻺴�
								 ) ) == NULL)
		{
			// ������ʾ������Ϣ
			sprintf_s(strbuf, "�ӿ�δ�ܴ򿪡�WinPcap��֧��%s��", IfInfo[i].DeviceName);
			// �ͷ��豸�б� 
			pcap_freealldevs(m_alldevs);
			return;
		}
	}

	// �������ݰ������̣߳���ȡ���ؽӿڵ�MAC��ַ���߳���ĿΪ��������
	CWinThread* pthread;
	for (i = 0; i < IfCount; i++)
	{
		pthread = AfxBeginThread(CaptureLocalARP, &IfInfo[i], THREAD_PRIORITY_NORMAL);
		if(!pthread)
		{
			MessageBox(L"�������ݰ������߳�ʧ�ܣ�");
			PostMessage(WM_QUIT, 0, 0);
		}
	}
	// ���б�������Ӳ����ַ��0
	for (i = 0; i < IfCount; i++)
	{
		setMAC(IfInfo[i].MACAddr, 0);
	}

	// Ϊ�õ���ʵ������ַ��ʹ����ٵ�MAC��ַ��IP��ַ�򱾻�����ARP����
	setMAC(srcMAC, 66); // ������ٵ�MAC��ַ
	srcIP = inet_addr("112.112.112.112"); // ������ٵ�IP��ַ
	for (i = 0; i < IfCount; i++){
		ARPRequest(IfInfo[i].adhandle, srcMAC, srcIP, IfInfo[i].ip[0].IPAddr);
	}
		
	j = 0;		

	// ȷ�����нӿڵ�MAC��ַ��ȫ�յ�
	setMAC(srcMAC, 0);
	do {
		Sleep(3000);	
		k = 0;
		for (i = 0; i < IfCount; i++)
		{
			if (!cmpMAC(IfInfo[i].MACAddr, srcMAC))
			{
				k++;
				continue;
			}
			else
			{
				break;
			}
		}
	} while (!((j++ > 10) || (k == IfCount)));

    if (k != IfCount)
	{
		MessageBox(L"������һ���ӿڵ�MAC��ַû�ܵõ���");
		PostMessage(WM_QUIT, 0, 0);
	}

	// ��־����ӿ���Ϣ
	for (i = 0; i < IfCount; i++)
	{
		m_log.InsertString(-1,L"�ӿ�:");
		m_log.InsertString(-1,L"  �豸����" + CString(IfInfo[i].DeviceName));
		m_log.InsertString(-1,L"  �豸������" + IfInfo[i].Description);
		m_log.InsertString(-1,(L"  MAC��ַ��" + MACntoa(IfInfo[i].MACAddr)));
		for (j = 0; j < IfInfo[i].ip.GetSize(); j++)
		{
			m_log.InsertString(-1,(L"    IP��ַ��"+IPntoa(IfInfo[i].ip[j].IPAddr)));
		}
	}

	// ��ʼ��·�ɱ���ʾ
	RouteTable_t rt;
	for (i = 0; i < IfCount; i++)
	{
		for (j = 0; j < IfInfo[i].ip.GetSize(); j++)
		{
			rt.IfNo = i;
			rt.DstIP = IfInfo[i].ip[j].IPAddr  & IfInfo[i].ip[j].IPMask;
			rt.Mask  = IfInfo[i].ip[j].IPMask;
			rt.NextHop = 0;	// ֱ��Ͷ��
			RouteTable.AddTail(rt);
			m_routeTable.InsertString(-1, IPntoa(rt.Mask) + " -- " + IPntoa(rt.DstIP) + " -- " + IPntoa(rt.NextHop) + "  (ֱ��Ͷ��)");
		}
	}

	// ���ù��˹���:��������arp��Ӧ֡����Ҫ·�ɵ�֡
	CString Filter, Filter0, Filter1;
	Filter1 = "(";
	for (i = 0; i < IfCount; i++)
	{
		Filter0 += L"(ether dst " + MACntoa(IfInfo[i].MACAddr) + L")";
		for (j = 0; j < IfInfo[i].ip.GetSize(); j++){
			Filter1 += L"(ip dst host " + IPntoa(IfInfo[i].ip[j].IPAddr) + L")";
			if (((j == (IfInfo[i].ip.GetSize() - 1))) && (i == (IfCount - 1))) 
				Filter1 += ")";
			else 
				Filter1 += " or ";
		}
		if (i != (IfCount-1))	
			Filter0 += " or ";
	}
	Filter = Filter0 + L" and ((arp and (ether[21]=0x2)) or (not" + Filter1 + L"))";

	for(int i=0;i<Filter.GetLength();i++)
	{
		strbuf[i]=char(Filter[i]);
	}
	strbuf[Filter.GetLength()]='\0';

	for (i = 0; i < IfCount; i++)
	{
		if ((pcap_compile(IfInfo[i].adhandle , &fcode, strbuf, 1, IfInfo[i].ip[0].IPMask) <0 )||(pcap_setfilter(IfInfo[i].adhandle, &fcode)<0))
		{
			MessageBox(Filter+L"���˹�����벻�ɹ���������д�Ĺ����﷨�Ƿ���ȷ�����ù���������");
			PostMessage(WM_QUIT,0,0);
		}
	}
	
	// ������Ҫ���豸�б�,�ͷ�֮
	pcap_freealldevs(m_alldevs);	

	TimerCount = 1;

	// ��ʼ�������ݰ�
	for (i=0; i < IfCount; i++)
	{
		pthread = AfxBeginThread(Capture, &IfInfo[i], THREAD_PRIORITY_NORMAL);
		if(!pthread)
		{
			MessageBox(L"�������ݰ������߳�ʧ�ܣ�");
			PostMessage(WM_QUIT, 0, 0);			
		}
	}
	m_log.AddString(L">>>>>>ת�����ݱ���ʼ��");
}


void Ctask3_2Dlg::OnBnClickedButtonAdd()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	bool flag;
	int i, j;
	DWORD ipaddr;
	RouteTable_t rt;

	m_next.GetAddress(ipaddr);
	ipaddr = htonl(ipaddr);

	// ���Ϸ���

	DWORD ipaddr1;
	DWORD ipaddr2;
	DWORD ipaddr3;
	POSITION pos, CurrentPos;
	// ��¼��������
	m_mask.GetAddress(ipaddr1);
	// ��¼Ŀ��IP
	m_dest.GetAddress(ipaddr2);
	// ��¼��һ��
	m_next.GetAddress(ipaddr3);
	pos = RouteTable.GetHeadPosition();
	for (i = 0; i < RouteTable.GetCount(); i++)
	{
		CurrentPos = pos;
		rt = RouteTable.GetNext(pos);
		if ((rt.Mask == htonl(ipaddr1)) && ((rt.Mask & rt.DstIP) == (rt.Mask & htonl(ipaddr2))) && (htonl(ipaddr3) != rt.NextHop))
		{
			MessageBox(L"��·���޷���ӣ����������룡");
			return;
		}
	}

	flag = false;
	for (i = 0; i < IfCount; i++)
	{
		for (j = 0; j < IfInfo[i].ip.GetSize(); j++)
		{
			if (((IfInfo[i].ip[j].IPAddr) & (IfInfo[i].ip[j].IPMask)) == ((IfInfo[i].ip[j].IPMask) & ipaddr))
			{
				rt.IfNo = i;
				// ��¼��������
				m_mask.GetAddress(ipaddr); 
				rt.Mask = htonl(ipaddr);
				// ��¼Ŀ��IP
				m_dest.GetAddress(ipaddr);
				rt.DstIP = htonl(ipaddr);
				// ��¼��һ��
				m_next.GetAddress(ipaddr);
				rt.NextHop = htonl(ipaddr);
				// �Ѹ���·�ɱ�����ӵ�·�ɱ�
				RouteTable.AddTail(rt);
				// ��·�ɱ�������ʾ��·�ɱ���
				m_routeTable.InsertString(-1, IPntoa(rt.Mask) + " -- " 
					+ IPntoa(rt.DstIP) + " -- " + IPntoa(rt.NextHop));
				flag = true;
			}
		}
	}
	if (!flag) 
	{
		MessageBox(L"����������������룡");
	}
}


void Ctask3_2Dlg::OnBnClickedButtonDelete()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	int	i;
	char str[100], ipaddr[20];
	ULONG mask, destination, nexthop;
	RouteTable_t rt;
	POSITION pos, CurrentPos;

	str[0] = NULL;
	ipaddr[0] = NULL;
	if ((i = m_routeTable.GetCurSel()) == LB_ERR) 
	{
		return;
	}

	CString STR;
	m_routeTable.GetText(i, STR);
	
	for(int i = 0; i < STR.GetLength(); i++)
		str[i] = STR[i];
	str[STR.GetLength()] = '\0';

	// ȡ����������ѡ��
	strncat_s(ipaddr, str, 15);
	mask = inet_addr(ipaddr);
	// ȡ��Ŀ�ĵ�ַѡ��
	ipaddr[0] = 0;
	strncat_s(ipaddr, &str[19], 15);
	destination = inet_addr(ipaddr);
	// ȡ����һ��ѡ��
	ipaddr[0] = 0;
	strncat_s(ipaddr, &str[38], 15);
	nexthop = inet_addr(ipaddr);

	if (nexthop == 0)
	{
		MessageBox(L"ֱ������·�ɣ�������ɾ����");
		return;
	}
	
	// �Ѹ�·�ɱ����·�ɱ�����ɾ��
	m_routeTable.DeleteString(i);

	// ·�ɱ���û����Ҫ��������ݣ��򷵻�
	if (RouteTable.IsEmpty()) 
	{
		return;
	}

	// ����·�ɱ�,����Ҫɾ����·�ɱ����·�ɱ���ɾ��
	pos = RouteTable.GetHeadPosition();	
	for (i=0; i<RouteTable.GetCount(); i++)
	{
		CurrentPos = pos;
		rt = RouteTable.GetNext(pos);

		if ((rt.Mask == mask) && (rt.DstIP == destination) && (rt.NextHop == nexthop))
		{
			RouteTable.RemoveAt(CurrentPos);
			return;
		}
	}
}


void Ctask3_2Dlg::OnBnClickedButtonBack()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_log.ResetContent(); // ���ԭ�п������
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	m_dev.EnableWindow(TRUE);
}

void Ctask3_2Dlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: �ڴ˴������Ϣ����������
	SP.RemoveAll();
	IP_MAC.RemoveAll();
	RouteTable.RemoveAll();

	for (int i=0; i<IfCount; i++)
	{
		IfInfo[i].ip.RemoveAll();
	}
}

void Ctask3_2Dlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	SendPacket_t sPacket;
	POSITION pos, CurrentPos;
	IPFrame_t *IPf;

	// û����Ҫ���������
	if (SP.IsEmpty()) 
	{
		return;	
	}
	
	mMutex.Lock(INFINITE);
	// ����ת��������
	pos = SP.GetHeadPosition();	
	for (int i = 0; i < SP.GetCount(); i++)
	{					
		CurrentPos = pos;
		sPacket = SP.GetNext(pos);
		if (sPacket.n_mTimer == nIDEvent)
		{
		    IPf = (IPFrame_t *)sPacket.PktData;
			// ��־�����Ϣ
			m_log.InsertString(-1, L"IP���ݱ���ת�������еȴ�10���δ�ܱ�ת��");
			m_log.InsertString(-1, (L"    ��ʱ����ɾ����IP���ݱ���" + IPntoa(IPf->IPHeader.SrcIP) + L"->" 
				+ IPntoa(IPf->IPHeader.DstIP) + "  " +  MACntoa(IPf->FrameHeader.SrcMAC) 
				+ L"->xx:xx:xx:xx:xx:xx"));

			KillTimer(sPacket.n_mTimer);
			SP.RemoveAt(CurrentPos);			
		}
	}
	mMutex.Unlock();
	
	CDialog::OnTimer(nIDEvent);
}

// ��ȡ���ؽӿ�MAC��ַ�߳�
UINT CaptureLocalARP(PVOID pParam)
{
	int res;
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	IfInfo_t *pIfInfo;
	ARPFrame_t *ARPFrame;
	CString DisplayStr;

	pIfInfo = (IfInfo_t *)pParam;

	while (true)
	{
		res = pcap_next_ex(pIfInfo->adhandle , &header, &pkt_data);
		// ��ʱ
        if (res == 0) 
			continue;           
		if (res > 0)
		{
			ARPFrame = (ARPFrame_t *) (pkt_data);
			// �õ����ӿڵ�MAC��ַ
			if ((ARPFrame->FrameHeader.FrameType == htons(0x0806)) 
				&& (ARPFrame->Operation == htons(0x0002)) 
				&& (ARPFrame->SendIP == pIfInfo->ip[0].IPAddr)) 
			{
				cpyMAC(pIfInfo->MACAddr, ARPFrame->SendHa);
				return 0;
			}
		}
	}
}

// ���ݰ������߳�
UINT Capture(PVOID pParam)
{
	int res;
	IfInfo_t *pIfInfo;
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	
	pIfInfo = (IfInfo_t *)pParam;

	// ��ʼ��ʽ���ղ�����֡
	while (true)	
	{
		res = pcap_next_ex( pIfInfo->adhandle, &header, &pkt_data);
			    
		if (res == 1)
		{
			FrameHeader_t *fh;
			fh = (FrameHeader_t *)pkt_data;
			switch (ntohs(fh->FrameType))
			{
				case 0x0806:				
					ARPFrame_t *ARPf;
					ARPf = (ARPFrame_t *)pkt_data;
					//pDlg->m_Log.AddString(L"�յ�ARP�� ԴIPΪ��"+IPntoa(ARPf->SendIP));					
					// ARP����ת��ARP��������
					ARPPacketProc(header, pkt_data);
					break;
				
				case 0x0800:					
					IPFrame_t *IPf;
					IPf = (IPFrame_t*) pkt_data;
					//pDlg->m_Log.AddString(L"�յ�IP�� ԴIPΪ��"+IPntoa(IPf->IPHeader.SrcIP));
					// IP����ת��IP��������
					IPPacketProc(pIfInfo, header, pkt_data);
					break;
				default: 
					break;
			}
		}
		else if (res == 0)	// ��ʱ
		{
			continue;
		}
		else
		{
			AfxMessageBox(L"pcap_next_ex��������!");
		}
	}
	return 0;
}

// ����ARP����
void ARPRequest(pcap_t *adhandle, UCHAR *srcMAC, ULONG srcIP, ULONG targetIP)
{
	ARPFrame_t ARPFrame;
	int i;

	for (i = 0; i < 6; i++)
	{
		ARPFrame.FrameHeader.DesMAC[i] = 255;
		ARPFrame.FrameHeader.SrcMAC[i] = srcMAC[i];
		ARPFrame.SendHa[i] = srcMAC[i];
		ARPFrame.RecvHa[i] = 0;
	}

	ARPFrame.FrameHeader.FrameType = htons(0x0806);
	ARPFrame.HardwareType = htons(0x0001);
	ARPFrame.ProtocolType = htons(0x0800);
	ARPFrame.HLen = 6;
	ARPFrame.PLen = 4;
	ARPFrame.Operation = htons(0x0001);
	ARPFrame.SendIP = srcIP;
	ARPFrame.RecvIP = targetIP;

    pcap_sendpacket(adhandle, (u_char *) &ARPFrame, sizeof(ARPFrame_t));
}

// ��IP��ַת���ɵ��ʮ������ʽ
CString IPntoa(ULONG nIPAddr)
{
	char strbuf[50];
	u_char *p;
	CString str;

	p = (u_char *)&nIPAddr;
	sprintf_s(strbuf,"%03d.%03d.%03d.%03d", p[0], p[1], p[2], p[3]);
	str = strbuf;
	return str;
}

// ��MAC��ַת���ɡ�%02X:%02X:%02X:%02X:%02X:%02X���ĸ�ʽ
CString MACntoa(UCHAR *nMACAddr)
{
	char strbuf[50];
	CString str;

	sprintf_s(strbuf,"%02X:%02X:%02X:%02X:%02X:%02X", nMACAddr[0], nMACAddr[1], 
		nMACAddr[2], nMACAddr[3], nMACAddr[4], nMACAddr[5]);
	str = strbuf;
	return str;
}

// �Ƚ�����MAC��ַ�Ƿ���ͬ
bool cmpMAC(UCHAR *MAC1, UCHAR *MAC2)
{
	for (int i=0; i<6; i++)
	{
		if (MAC1[i]==MAC2[i]) 
			continue;
		else 
			return false;
	}
	return true;
}

void cpyMAC(UCHAR *MAC1, UCHAR *MAC2)
{
	for (int i=0; i<6; i++) 
		MAC1[i]=MAC2[i];
}

void setMAC(UCHAR *MAC, UCHAR ch)
{
	for (int i=0; i<6; i++) 
		MAC[i] = ch;
	return;
}

// ��ѯIP-MACӳ���
bool IPLookup(ULONG ipaddr, UCHAR *p)
{
	IP_MAC_t ip_mac;
	POSITION pos;

	if (IP_MAC.IsEmpty())
		return false;

	pos = IP_MAC.GetHeadPosition();
    for (int i = 0; i < IP_MAC.GetCount(); i++)
	{
        ip_mac = IP_MAC.GetNext(pos);
		if (ipaddr == ip_mac.IPAddr)
		{
			for (int j = 0; j < 6; j++)
			{
				p[j] = ip_mac.MACAddr[j];
			}
			return true;
		}
	}
	return false;
}

// ����ARP���ݰ�
void ARPPacketProc(struct pcap_pkthdr *header, const u_char *pkt_data)
{
	bool flag;
	ARPFrame_t *ARPf;
	IPFrame_t *IPf;
	SendPacket_t sPacket;
	POSITION pos, CurrentPos;
	IP_MAC_t ip_mac;
	UCHAR macAddr[6];
	
	ARPf = (ARPFrame_t *)pkt_data;
	
	if (ARPf->Operation == ntohs(0x0002))
	{
		//pDlg->m_log.AddString(L"----------------------------------------------------------------------------------");
		pDlg->m_log.InsertString(-1, L"�յ�ARP��Ӧ��");
		pDlg->m_log.InsertString(-1, (L"   ARP "+(IPntoa(ARPf->SendIP))+L" -- "
				+MACntoa(ARPf->SendHa)));
		// IP��MAC��ַӳ������Ѿ����ڸö�Ӧ��ϵ
		if (IPLookup(ARPf->SendIP, macAddr)) 
		{
			pDlg->m_log.InsertString(-1, L"   �ö�Ӧ��ϵ�Ѿ�������IP��MAC��ַӳ�����");
			return;	
		}		
		else
		{
			ip_mac.IPAddr = ARPf->SendIP;	
			memcpy(ip_mac.MACAddr, ARPf->SendHa, 6);
			// ��IP-MACӳ���ϵ�������		
			IP_MAC.AddHead(ip_mac);

			// ��־�����Ϣ
			pDlg->m_log.InsertString(-1,L"   ���ö�Ӧ��ϵ����IP��MAC��ַӳ�����");
		}
	
		mMutex.Lock(INFINITE);
		do{		
			// �鿴�Ƿ���ת�������е�IP���ݱ�
			flag = false;

			// û����Ҫ���������
			if (SP.IsEmpty()) 
				break;	

			// ����ת��������
			pos = SP.GetHeadPosition();	
			for (int i=0; i < SP.GetCount(); i++)
			{					
				CurrentPos = pos;
				sPacket = SP.GetNext(pos);

				if (sPacket.TargetIP == ARPf->SendIP)
				{
					IPf = (IPFrame_t *)sPacket.PktData;
					cpyMAC(IPf->FrameHeader.DesMAC, ARPf->SendHa);

					for(int t = 0; t < 6; t++)
					{
						IPf->FrameHeader.SrcMAC[t] = IfInfo[sPacket.IfNo].MACAddr[t];
					}
					// ����IP���ݰ�
					pcap_sendpacket(IfInfo[sPacket.IfNo].adhandle, (u_char *) sPacket.PktData, sPacket.len);

					SP.RemoveAt(CurrentPos);
					
					// ��־�����Ϣ
					pDlg->m_log.InsertString(-1, L"   ת����������Ŀ�ĵ�ַ�Ǹ�MAC��ַ��IP���ݰ�");
					pDlg->m_log.InsertString(-1, (L"     ����IP���ݰ���"+IPntoa(IPf->IPHeader.SrcIP) + L"->" 
						+ IPntoa(IPf->IPHeader.DstIP) + "  " + MACntoa(IPf->FrameHeader.SrcMAC )
						+L"->"+MACntoa(IPf->FrameHeader.DesMAC)));

					flag = true;
					break;
				}
			}
		} while(flag);

		mMutex.Unlock();
	}
}

// ��ѯ·�ɱ�
DWORD RouteLookup(UINT &ifNO, DWORD desIP, CList <RouteTable_t, RouteTable_t&> *routeTable)
{
	// desIPΪ������
	DWORD MaxMask = 0; // ���������������ĵ�ַ��û�л��ʱ��ʼ��Ϊ-1
	int Index = -1; // ���������������ĵ�ַ��Ӧ��·�ɱ��������Ա�����һվ·�����ĵ�ַ

	POSITION pos;
	RouteTable_t rt;
	DWORD tmp;

	pos = routeTable->GetHeadPosition(); 
	for (int i=0; i < routeTable->GetCount(); i++)
	{
		rt = routeTable->GetNext(pos);
		if ((desIP & rt.Mask) == rt.DstIP)
		{
			Index = i;

			if(rt.Mask >= MaxMask)
			{
				MaxMask = rt.Mask;
				ifNO = rt.IfNo;

				if (rt.NextHop == 0)	// ֱ��Ͷ��
				{
					tmp = desIP;
				}
				else
				{	
					tmp = rt.NextHop;
				}
			}	
		}
	}

	if(Index == -1)				// Ŀ�Ĳ��ɴ�
	{
		return -1;
	}
	else		// �ҵ�����һ����ַ
	{
		return tmp;
	}		
}

// ����IP���ݰ�
void IPPacketProc(IfInfo_t *pIfInfo, struct pcap_pkthdr *header, const u_char *pkt_data)
{
	//pDlg->m_log.AddString(L"----------------------------------------------------------------------------------");
	IPFrame_t *IPf;
	SendPacket_t sPacket;

	IPf = (IPFrame_t *) pkt_data;
	
	pDlg->m_log.InsertString(-1, (L"�յ�IP���ݰ�:" + IPntoa(IPf->IPHeader.SrcIP) + L"->" 
		+ IPntoa(IPf->IPHeader.DstIP)));
	
	// ICMP��ʱ
	if (IPf->IPHeader.TTL <= 1)///////<>if (IPf->IPHeader.TTL <= 1)
	{
		ICMPPacketProc(pIfInfo, 11, 0, pkt_data);
		return;
	}

	IPHeader_t *IpHeader = &(IPf->IPHeader);
	// ICMP���
	if (IsChecksumRight((char *)IpHeader) == 0)
	{
		// ��־�����Ϣ
		pDlg->m_log.InsertString(-1, L"   IP���ݰ���ͷУ��ʹ��󣬶������ݰ�");
		return;
	}

	// ·�ɲ�ѯ
	DWORD nextHop; // ����·��ѡ���㷨�õ�����һվĿ��IP��ַ
	UINT ifNo; // ��һ���Ľӿ����
	// ·�ɲ�ѯ
	if((nextHop = RouteLookup(ifNo, IPf->IPHeader.DstIP, &RouteTable)) == -1)
	{
		// ICMPĿ�Ĳ��ɴ�
		ICMPPacketProc(pIfInfo, 3, 0, pkt_data);
		return;
	}
	else
	{
		sPacket.IfNo = ifNo;
		sPacket.TargetIP = nextHop;

		cpyMAC(IPf->FrameHeader.SrcMAC, IfInfo[sPacket.IfNo].MACAddr);

		// TTL��1
		IPf->IPHeader.TTL -= 1;
			
		unsigned short check_buff[sizeof(IPHeader_t)];
		// ��IPͷ�е�У���Ϊ0
		IPf->IPHeader.Checksum = 0;                                      
	
		memset(check_buff, 0, sizeof(IPHeader_t));
		IPHeader_t * ip_header = &(IPf->IPHeader);
		memcpy(check_buff, ip_header, sizeof(IPHeader_t));
 
		// ����IPͷ��У���
		IPf->IPHeader.Checksum = ChecksumCompute(check_buff, sizeof(IPHeader_t));
		
		// IP-MAC��ַӳ����д��ڸ�ӳ���ϵ
		if (IPLookup(sPacket.TargetIP, IPf->FrameHeader.DesMAC)) 
		{
			memcpy(sPacket.PktData, pkt_data, header->len);
			sPacket.len = header->len;
			if(pcap_sendpacket(IfInfo[sPacket.IfNo].adhandle, (u_char *) sPacket.PktData, sPacket.len) != 0)
			{
				// ������
				AfxMessageBox(L"����IP���ݰ�ʱ����!");
				return;
			}

			// ��־�����Ϣ
			pDlg->m_log.InsertString(-1,L"   ת��IP���ݰ���");
			pDlg->m_log.InsertString(-1,(L"   "+IPntoa(IPf->IPHeader.SrcIP) + L"->" 
				+ IPntoa(IPf->IPHeader.DstIP) + "  " + MACntoa(IPf->FrameHeader.SrcMAC )
				+ L"->" + MACntoa(IPf->FrameHeader.DesMAC))); 
		}
		// IP-MAC��ַӳ����в����ڸ�ӳ���ϵ
		else
		{
			if (SP.GetCount() < 65530)		// ���뻺�����
			{
				sPacket.len = header->len;	
				// ����Ҫת�������ݱ����뻺����
				memcpy(sPacket.PktData, pkt_data, header->len);
						
				// ��ĳһʱ��ֻ����һ���߳�ά������
				mMutex.Lock(INFINITE);	

				sPacket.n_mTimer = TimerCount;
				if (TimerCount++ > 65533) 
				{
					TimerCount = 1;
				}
				pDlg->SetTimer(sPacket.n_mTimer, 10000, NULL);
				SP.AddTail(sPacket);
			
				mMutex.Unlock();

				// ��־�����Ϣ
				pDlg->m_log.InsertString(-1,L"   ȱ��Ŀ��MAC��ַ����IP���ݰ�����ת��������");
				pDlg->m_log.InsertString(-1, (L"   ����ת�������������ݰ�Ϊ��"+IPntoa(IPf->IPHeader.SrcIP) 
					+ L"->" + IPntoa(IPf->IPHeader.DstIP) + L"  " + MACntoa(IPf->FrameHeader.SrcMAC)
					+ L"->xx:xx:xx:xx:xx:xx")); 
				pDlg->m_log.InsertString(-1, L"   ����ARP����");

				// ����ARP����
				ARPRequest(IfInfo[sPacket.IfNo].adhandle, IfInfo[sPacket.IfNo].MACAddr, 
					IfInfo[sPacket.IfNo].ip[1].IPAddr, sPacket.TargetIP);
			}	
			else // �绺�����̫���������ñ�
			{
				// ��־�����Ϣ
				pDlg->m_log.InsertString(-1, L"   ת�����������������IP���ݰ�");
				pDlg->m_log.InsertString(-1, (L"   ������IP���ݰ�Ϊ��" + IPntoa(IPf->IPHeader.SrcIP) + L"->" 
					+ IPntoa(IPf->IPHeader.DstIP) + L"  " + MACntoa(IPf->FrameHeader.SrcMAC)
					+ L"->xx:xx:xx:xx:xx:xx")); 
			}
		}	
	}
}

// ����ICMP���ݰ�
void ICMPPacketProc(IfInfo_t *pIfInfo, BYTE type, BYTE code, const u_char *pkt_data)
{
	u_char * ICMPBuf = new u_char[70];

	// ���֡�ײ�
	memcpy(((FrameHeader_t *)ICMPBuf)->DesMAC, ((FrameHeader_t *)pkt_data)->SrcMAC, 6);
	memcpy(((FrameHeader_t *)ICMPBuf)->SrcMAC, ((FrameHeader_t *)pkt_data)->DesMAC, 6);
	((FrameHeader_t *)ICMPBuf)->FrameType = htons(0x0800);

	// ���IP�ײ�
	((IPHeader_t *)(ICMPBuf+14))->Ver_HLen = ((IPHeader_t *)(pkt_data+14))->Ver_HLen;
	((IPHeader_t *)(ICMPBuf+14))->TOS = ((IPHeader_t *)(pkt_data+14))->TOS;
	((IPHeader_t *)(ICMPBuf+14))->TotalLen = htons(56);
	((IPHeader_t *)(ICMPBuf+14))->ID = ((IPHeader_t *)(pkt_data+14))->ID;
	((IPHeader_t *)(ICMPBuf+14))->Flag_Segment = ((IPHeader_t *)(pkt_data+14))->Flag_Segment;
	((IPHeader_t *)(ICMPBuf+14))->TTL = 64;
	((IPHeader_t *)(ICMPBuf+14))->Protocol = 1;
	//((IPHeader_t *)(ICMPBuf+14))->SrcIP = ((IPHeader_t *)(pkt_data+14))->DstIP;
	((IPHeader_t *)(ICMPBuf+14))->SrcIP = pIfInfo->ip[1].IPAddr;
	((IPHeader_t *)(ICMPBuf+14))->DstIP = ((IPHeader_t *)(pkt_data+14))->SrcIP;
	((IPHeader_t *)(ICMPBuf+14))->Checksum = htons(ChecksumCompute((unsigned short *)(ICMPBuf+14),20));

	// ���ICMP�ײ�
	((ICMPHeader_t *)(ICMPBuf+34))->Type = type;
	((ICMPHeader_t *)(ICMPBuf+34))->Code = code;
	((ICMPHeader_t *)(ICMPBuf+34))->Id = 0;
	((ICMPHeader_t *)(ICMPBuf+34))->Sequence = 0;
	((ICMPHeader_t *)(ICMPBuf+34))->Checksum = htons(ChecksumCompute((unsigned short *)(ICMPBuf+34),8));
	
	// �������
	memcpy((u_char *)(ICMPBuf+42),(IPHeader_t *)(pkt_data+14),20);
	memcpy((u_char *)(ICMPBuf+62),(u_char *)(pkt_data+34),8);

	// �������ݰ�
	pcap_sendpacket(pIfInfo->adhandle, (u_char *)ICMPBuf, 70 );
	
	// ��־�����Ϣ
	if (type == 11)
	{
		pDlg->m_log.InsertString(-1, L"   ����ICMP��ʱ���ݰ���");
	}
	if (type == 3)
	{
		pDlg->m_log.InsertString(-1, L"   ����ICMPĿ�Ĳ��ɴ����ݰ���");
	}
	pDlg->m_log.InsertString(-1, (L"   ICMP ->" + IPntoa(((IPHeader_t *)(ICMPBuf+14))->DstIP)
		+ L"-" + MACntoa(((FrameHeader_t *)ICMPBuf)->DesMAC)));

	delete [] ICMPBuf;
}

// �ж�IP���ݰ�ͷ��У����Ƿ���ȷ
int IsChecksumRight(char * buffer)
{
	// ���IPͷ����
	IPHeader_t * ip_header = (IPHeader_t *)buffer;  
	// ����ԭ����У���
	unsigned short checksumBuf = ip_header->Checksum;              
	unsigned short check_buff[sizeof(IPHeader_t)];
	// ��IPͷ�е�У���Ϊ0
	ip_header->Checksum = 0;
	
	memset(check_buff, 0, sizeof(IPHeader_t));
	memcpy(check_buff, ip_header, sizeof(IPHeader_t));

	// ����IPͷ��У���
	ip_header->Checksum = ChecksumCompute(check_buff, sizeof(IPHeader_t));   

	// �뱸�ݵ�У��ͽ��бȽ�
	if (ip_header->Checksum == checksumBuf)
		return 1;
	else
		return 0;
}

// ����У���
unsigned short ChecksumCompute(unsigned short * buffer,int size)   
{
	// 32λ���ӳٽ�λ
	unsigned long cksum = 0;                                        
	while (size > 1)
	{
		cksum += * buffer++;
		// 16λ���
		size -= sizeof(unsigned short);                             
	}
	if(size)
	{
		// �������е���8λ
		cksum += *(unsigned char *)buffer;
	}
	// ����16λ��λ������16λ
	cksum = (cksum >> 16) + (cksum & 0xffff);                       
	cksum += (cksum >> 16);
	// ȡ��
	return (unsigned short)(~cksum);                                
}