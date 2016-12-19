// OptFlowView.h : interface of the OptFlowView class
//

#pragma once

#include <vector>
#include <xmemory>

#include "OglContext.h"
#include "GlView.h"

#include "opencv2/videoio.hpp"

// OptFlowView 

class TestView : public CWnd
{
// Construction
public:
	TestView();
	virtual ~TestView();

// Attributes
public:

protected:
	// Overrides
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// Generated message map functions
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()

private:
	void initGL();
	void createOpenGLContext();

private:
	cv::Mat mFrame;
	cv::VideoCapture mCamera;

	std::unique_ptr<cl::Context> mClContext;
	std::unique_ptr<cl::CommandQueue> mClQueue;

	std::unique_ptr<GlView> mGpu;
	std::unique_ptr<Ogl::WinGlContext> mGlCtxt;
};
