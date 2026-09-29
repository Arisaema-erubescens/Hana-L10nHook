#include "Hook.h"



#include <Windows.h>



#include "DetoursHook.h"

#include "Encoding.h"

#include "Logger.h"

#include "ModuleUtils.h"

#include "QStringBridge.h"

#include "TranslationManager.h"

#include "Win32GdiHook.h"



#include <algorithm>
#include <cstddef>

#include <cwchar>

#include <string>

#include <string_view>
#include <unordered_set>
#include <vector>



using QPainterDrawText3_t = void (*)(void* pThis, void* pointF, const void* text, int textFlags, int justificationPadding);

using QPainterDrawText4_t = void (*)(void* pThis, void* rect, int flags, const void* text, void* boundingRect);

using QPainterDrawText5_t = void (*)(void* pThis, void* rectangle, const void* text, void* option);

using QPainterDrawText6_t = void (*)(void* pThis, void* rectangle, int flags, const void* text, void* boundingRect);

using QPainterDrawText7_t = void (*)(void* pThis, int x, int y, int width, int height, int flags, const void* text, void* boundingRect);

using QPainterDrawTextPoint_t = void (*)(void* pThis, const void* point, const void* text);

using QPainterDrawTextPointF_t = void (*)(void* pThis, const void* point, const void* text);

using QPainterDrawTextXY_t = void (*)(void* pThis, int x, int y, const void* text);

QPainterDrawText3_t OriginalDrawText3 = nullptr;

QPainterDrawText4_t OriginalDrawText4 = nullptr;

QPainterDrawText5_t OriginalDrawText5 = nullptr;

QPainterDrawText6_t OriginalDrawText6 = nullptr;

QPainterDrawText7_t OriginalDrawText7 = nullptr;

QPainterDrawTextPoint_t OriginalDrawTextPoint = nullptr;

QPainterDrawTextPointF_t OriginalDrawTextPointF = nullptr;

QPainterDrawTextXY_t OriginalDrawTextXY = nullptr;

void HookedDrawText3(void* pThis, void* pointF, const void* text, int textFlags, int justificationPadding);

void HookedDrawText4(void* pThis, void* rect, int flags, const void* text, void* boundingRect);

void HookedDrawText5(void* pThis, void* rectangle, const void* text, void* option);

void HookedDrawText6(void* pThis, void* rectangle, int flags, const void* text, void* boundingRect);

void HookedDrawText7(void* pThis, int x, int y, int width, int height, int flags, const void* text, void* boundingRect);

void HookedDrawTextPoint(void* pThis, const void* point, const void* text);

void HookedDrawTextPointF(void* pThis, const void* point, const void* text);

void HookedDrawTextXY(void* pThis, int x, int y, const void* text);



using QLabelSetText_t = void (*)(void* pThis, const void* text);

QLabelSetText_t OriginalQLabelSetText = nullptr;

void HookedQLabelSetText(void* pThis, const void* text);

using QAbstractButtonSetText_t = void (*)(void* pThis, const void* text);
using QActionSetText_t = void (*)(void* pThis, const void* text);
using QGroupBoxSetTitle_t = void (*)(void* pThis, const void* text);
using QWidgetSetWindowTitle_t = void (*)(void* pThis, const void* text);

QAbstractButtonSetText_t OriginalQAbstractButtonSetText = nullptr;
QActionSetText_t OriginalQActionSetText = nullptr;
QGroupBoxSetTitle_t OriginalQGroupBoxSetTitle = nullptr;
QWidgetSetWindowTitle_t OriginalQWidgetSetWindowTitle = nullptr;

void HookedQAbstractButtonSetText(void* pThis, const void* text);
void HookedQActionSetText(void* pThis, const void* text);
void HookedQGroupBoxSetTitle(void* pThis, const void* text);
void HookedQWidgetSetWindowTitle(void* pThis, const void* text);

using QComboBoxSetItemText_t = void (*)(void* pThis, int index, const void* text);
using QToolBoxSetItemText_t = void (*)(void* pThis, int index, const void* text);
using QTabBarSetTabText_t = void (*)(void* pThis, int index, const void* text);
using QTabWidgetSetTabText_t = void (*)(void* pThis, int index, const void* text);
using QWidgetSetPlaceholderText_t = void (*)(void* pThis, const void* text);

QComboBoxSetItemText_t OriginalQComboBoxSetItemText = nullptr;
QToolBoxSetItemText_t OriginalQToolBoxSetItemText = nullptr;
QTabBarSetTabText_t OriginalQTabBarSetTabText = nullptr;
QTabWidgetSetTabText_t OriginalQTabWidgetSetTabText = nullptr;
QWidgetSetPlaceholderText_t OriginalQComboBoxSetPlaceholderText = nullptr;
QWidgetSetPlaceholderText_t OriginalQLineEditSetPlaceholderText = nullptr;
QWidgetSetPlaceholderText_t OriginalQPlainTextEditSetPlaceholderText = nullptr;
QWidgetSetPlaceholderText_t OriginalQTextEditSetPlaceholderText = nullptr;

void HookedQComboBoxSetItemText(void* pThis, int index, const void* text);
void HookedQToolBoxSetItemText(void* pThis, int index, const void* text);
void HookedQTabBarSetTabText(void* pThis, int index, const void* text);
void HookedQTabWidgetSetTabText(void* pThis, int index, const void* text);
void HookedQComboBoxSetPlaceholderText(void* pThis, const void* text);
void HookedQLineEditSetPlaceholderText(void* pThis, const void* text);
void HookedQPlainTextEditSetPlaceholderText(void* pThis, const void* text);
void HookedQTextEditSetPlaceholderText(void* pThis, const void* text);

using QTextDocumentSetHtml_t = void (*)(void* pThis, const void* text);
using QTextDocumentSetPlainText_t = void (*)(void* pThis, const void* text);
using QTextEditSetHtml_t = void (*)(void* pThis, const void* text);
using QTextEditInsertHtml_t = void (*)(void* pThis, const void* text);
using QTextEditSetPlainText_t = void (*)(void* pThis, const void* text);
using QPlainTextEditSetPlainText_t = void (*)(void* pThis, const void* text);
using QMessageBoxSetText_t = void (*)(void* pThis, const void* text);
using QMessageBoxSetInformativeText_t = void (*)(void* pThis, const void* text);
using QMessageBoxSetDetailedText_t = void (*)(void* pThis, const void* text);
using QWidgetSetStatusTip_t = void (*)(void* pThis, const void* text);
using QActionSetStatusTip_t = void (*)(void* pThis, const void* text);
using QActionSetToolTip_t = void (*)(void* pThis, const void* text);
using QActionSetWhatsThis_t = void (*)(void* pThis, const void* text);

QTextDocumentSetHtml_t OriginalQTextDocumentSetHtml = nullptr;
QTextDocumentSetPlainText_t OriginalQTextDocumentSetPlainText = nullptr;
QTextEditSetHtml_t OriginalQTextEditSetHtml = nullptr;
QTextEditInsertHtml_t OriginalQTextEditInsertHtml = nullptr;
QTextEditSetPlainText_t OriginalQTextEditSetPlainText = nullptr;
QPlainTextEditSetPlainText_t OriginalQPlainTextEditSetPlainText = nullptr;
QMessageBoxSetText_t OriginalQMessageBoxSetText = nullptr;
QMessageBoxSetInformativeText_t OriginalQMessageBoxSetInformativeText = nullptr;
QMessageBoxSetDetailedText_t OriginalQMessageBoxSetDetailedText = nullptr;
QWidgetSetStatusTip_t OriginalQWidgetSetStatusTip = nullptr;
QActionSetStatusTip_t OriginalQActionSetStatusTip = nullptr;
QActionSetToolTip_t OriginalQActionSetToolTip = nullptr;
QActionSetWhatsThis_t OriginalQActionSetWhatsThis = nullptr;

void HookedQTextDocumentSetHtml(void* pThis, const void* text);
void HookedQTextDocumentSetPlainText(void* pThis, const void* text);
void HookedQTextEditSetHtml(void* pThis, const void* text);
void HookedQTextEditInsertHtml(void* pThis, const void* text);
void HookedQTextEditSetPlainText(void* pThis, const void* text);
void HookedQPlainTextEditSetPlainText(void* pThis, const void* text);
void HookedQMessageBoxSetText(void* pThis, const void* text);
void HookedQMessageBoxSetInformativeText(void* pThis, const void* text);
void HookedQMessageBoxSetDetailedText(void* pThis, const void* text);
void HookedQWidgetSetStatusTip(void* pThis, const void* text);
void HookedQActionSetStatusTip(void* pThis, const void* text);
void HookedQActionSetToolTip(void* pThis, const void* text);
void HookedQActionSetWhatsThis(void* pThis, const void* text);

using QWidgetSetToolTip_t = void (*)(void* pThis, const void* text);
using QWidgetAddActionObject_t = void (*)(void* pThis, void* action);
using QWidgetRemoveAction_t = void (*)(void* pThis, void* action);

QWidgetSetToolTip_t OriginalQWidgetSetToolTip = nullptr;
QWidgetAddActionObject_t OriginalQWidgetAddActionObject = nullptr;
QWidgetRemoveAction_t OriginalQWidgetRemoveAction = nullptr;

void HookedQWidgetSetToolTip(void* pThis, const void* text);
void HookedQWidgetAddActionObject(void* pThis, void* action);

using QMainWindowSetMenuBar_t = void (*)(void* pThis, void* menuBar);
using QMainWindowSetCentralWidget_t = void (*)(void* pThis, void* widget);
using QMenuBarAddMenu_t = void* (*)(void* pThis, const void* title);
using QMenuBarAddMenuObject_t = void* (*)(void* pThis, void* menu);
using QMenuSetTitle_t = void (*)(void* pThis, const void* title);
using QMenuTitle_t = void* (*)(void* result, const void* pThis);


QMainWindowSetMenuBar_t OriginalQMainWindowSetMenuBar = nullptr;
QMainWindowSetCentralWidget_t OriginalQMainWindowSetCentralWidget = nullptr;
QMenuBarAddMenu_t OriginalQMenuBarAddMenu = nullptr;
QMenuBarAddMenuObject_t OriginalQMenuBarAddMenuObject = nullptr;
QMenuSetTitle_t OriginalQMenuSetTitle = nullptr;
QMenuTitle_t OriginalQMenuTitle = nullptr;


void HookedQMainWindowSetMenuBar(void* pThis, void* menuBar);
void HookedQMainWindowSetCentralWidget(void* pThis, void* widget);
void* HookedQMenuBarAddMenu(void* pThis, const void* title);
void* HookedQMenuBarAddMenuObject(void* pThis, void* menu);
void HookedQMenuSetTitle(void* pThis, const void* title);



using QMenuBarAddAction_t = void* (*)(void* pThis, const void* text);
using QMenuBarAddActionObject_t = void* (*)(void* pThis, void* action);
using QActionText_t = void* (*)(void* result, const void* pThis);

QMenuBarAddAction_t OriginalQMenuBarAddAction = nullptr;
QMenuBarAddActionObject_t OriginalQMenuBarAddActionObject = nullptr;
QActionText_t OriginalQActionText = nullptr;

using QWidgetActions_t = void* (*)(void* result, const void* pThis);
using QActionMenu_t = void* (*)(const void* pThis);

QWidgetActions_t OriginalQWidgetActions = nullptr;
QActionMenu_t OriginalQActionMenu = nullptr;

void* HookedQMenuBarAddAction(void* pThis, const void* text);
void* HookedQMenuBarAddActionObject(void* pThis, void* action);

using QActionActivate_t = void (*)(void* pThis, int event);
using QActionTrigger_t = void (*)(void* pThis);

QActionActivate_t OriginalQActionActivate = nullptr;
QActionTrigger_t OriginalQActionTrigger = nullptr;

void HookedQActionActivate(void* pThis, int event);
void HookedQActionTrigger(void* pThis);

static void* g_languageAction = nullptr;
static void MoveLanguageActionAfterHelp(void* menuBar);
static void AddLanguageAction(void* menuBar);
static bool IsHelpActionObject(void* action);
static void* g_helpMenu = nullptr;
static void* g_menuBar = nullptr;
static constexpr wchar_t kLanguageActionText[] = L"中/英切换 Hana汉化";
static void RefreshMenuActions();




using QWindowSetTitle_t = void (*)(void* pThis, const void* text);

QWindowSetTitle_t OriginalQWindowSetTitle = nullptr;

void HookedQWindowSetTitle(void* pThis, const void* text);



using QFileDialogGetOpenFileName_t = void* (*)(void* outQString, void* parent, const void* caption, const void* dir, const void* filter, void* selectedFilter, unsigned int options);

QFileDialogGetOpenFileName_t OriginalQFileDialogGetOpenFileName = nullptr;

void* HookedQFileDialogGetOpenFileName(void* outQString, void* parent, const void* caption, const void* dir, const void* filter, void* selectedFilter, unsigned int options);



using QFileDialogGetExistingDirectory_t = void* (*)(void* outQString, void* parent, const void* caption, const void* dir, unsigned int options);

QFileDialogGetExistingDirectory_t OriginalQFileDialogGetExistingDirectory = nullptr;

void* HookedQFileDialogGetExistingDirectory(void* outQString, void* parent, const void* caption, const void* dir, unsigned int options);



using QFontMetricsSize_t = void* (*)(void* pThis, void* result, int flags, void* text, int tabStops, int* tabArray);

QFontMetricsSize_t OriginalQFontMetricsSize = nullptr;

void* HookedFontMetricsSize(void* pThis, void* result, int flags, void* text, int tabStops, int* tabArray);

using QFontMetricsElidedText_t = void* (*)(void* result, void* pThis, const void* text, int mode, int width, int flags);

QFontMetricsElidedText_t OriginalQFontMetricsElidedText = nullptr;

void* HookedFontMetricsElidedText(void* result, void* pThis, const void* text, int mode, int width, int flags);

using QFontMetricsFElidedText_t = void* (*)(void* result, void* pThis, const void* text, int mode, double width, int flags);

QFontMetricsFElidedText_t OriginalQFontMetricsFElidedText = nullptr;

void* HookedFontMetricsFElidedText(void* result, void* pThis, const void* text, int mode, double width, int flags);



static DetoursHook detoursHook_;     // DetoursHook 对象

static QStringBridge qstringBridge_; // QString 桥接对象



static QStringBridge::ScopedQString QStringTranslation(

    const void* text,

    const char* hookName,

    bool writeUntranslated = true

);



bool IsCompatibleQtModule(const wchar_t* moduleName, const std::wstring& qtCoreModuleName) {

    if (moduleName == nullptr) {

        return false;

    }

    if (qtCoreModuleName.find(L"Qt6") != std::wstring::npos) {

        return wcsstr(moduleName, L"Qt6") != nullptr;

    }

    if (qtCoreModuleName.find(L"Qt5") != std::wstring::npos) {

        return wcsstr(moduleName, L"Qt5") != nullptr;

    }

    return false;

}



std::wstring DetectQtCoreModuleName() {

    if (ModuleUtils::LoadLibraryFromProcessDirectory(L"Qt6Core.dll") != nullptr) {

        return L"Qt6Core.dll";

    }

    if (ModuleUtils::LoadLibraryFromProcessDirectory(L"Qt5Core.dll") != nullptr) {

        return L"Qt5Core.dll";

    }

    return {};

}



struct HookExportCandidate {

    const wchar_t* moduleName;

    const char* functionName;

};



constexpr std::size_t kMaxHookCandidateCount = 4;



struct HookFunctionInfo {

    void** original;

    void* detour;

    HookExportCandidate candidates[kMaxHookCandidateCount];

};



bool RegisterHookFunction(const HookFunctionInfo& hook, const std::wstring& qtCoreModuleName) {

    if (hook.original == nullptr || hook.detour == nullptr) {

        Logger::Write(L"[Hook] 注册候选 Hook 参数无效。original=%p detour=%p", hook.original, hook.detour);

        return false;

    }



    *hook.original = nullptr;

    for (const HookExportCandidate& candidate : hook.candidates) {

        if (candidate.moduleName == nullptr || candidate.functionName == nullptr) {

            continue;

        }

        if (!IsCompatibleQtModule(candidate.moduleName, qtCoreModuleName)) {

            continue;

        }



        HMODULE module = ModuleUtils::LoadLibraryFromProcessDirectory(candidate.moduleName);

        if (module == nullptr) {

            continue;

        }



        void* address = reinterpret_cast<void*>(GetProcAddress(module, candidate.functionName));

        if (address == nullptr) {

            continue;

        }



        *hook.original = address;

        if (!detoursHook_.Register(hook.original, hook.detour)) {

            Logger::Write(L"[Hook] 注册候选 Hook 失败。module=%s function=%s address=%p", candidate.moduleName, U82W(candidate.functionName), address);

            *hook.original = nullptr;

            return false;

        }



        Logger::Write(L"[Hook] 注册候选 Hook 成功。module=%s function=%s address=%p", candidate.moduleName, U82W(candidate.functionName), address);

        return true;

    }



    Logger::Write(L"[Hook] 未找到候选 Hook 导出。function=%s", U82W(hook.candidates[0].functionName));

    return false;

}



const HookFunctionInfo kHookFunctions[] = {

    {

        reinterpret_cast<void**>(&OriginalDrawText3),

        reinterpret_cast<void*>(HookedDrawText3),

        {

            {L"Qt5Gui.dll", "?drawText@QPainter@QT@@QEAAXAEBVQPointF@2@AEBVQString@2@HH@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@QT@@QEAAXAEBVQPointF@2@AEBVQString@2@HH@Z"},

            {L"Qt5Gui.dll", "?drawText@QPainter@@QEAAXAEBVQPointF@@AEBVQString@@HH@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@@QEAAXAEBVQPointF@@AEBVQString@@HH@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalDrawText4),

        reinterpret_cast<void*>(HookedDrawText4),

        {

            {L"Qt5Gui.dll", "?drawText@QPainter@QT@@QEAAXAEBVQRect@2@HAEBVQString@2@PEAV32@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@QT@@QEAAXAEBVQRect@2@HAEBVQString@2@PEAV32@@Z"},

            {L"Qt5Gui.dll", "?drawText@QPainter@@QEAAXAEBVQRect@@HAEBVQString@@PEAV2@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@@QEAAXAEBVQRect@@HAEBVQString@@PEAV2@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalDrawText5),

        reinterpret_cast<void*>(HookedDrawText5),

        {

            {L"Qt5Gui.dll", "?drawText@QPainter@QT@@QEAAXAEBVQRectF@2@AEBVQString@2@AEBVQTextOption@2@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@QT@@QEAAXAEBVQRectF@2@AEBVQString@2@AEBVQTextOption@2@@Z"},

            {L"Qt5Gui.dll", "?drawText@QPainter@@QEAAXAEBVQRectF@@AEBVQString@@AEBVQTextOption@@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@@QEAAXAEBVQRectF@@AEBVQString@@AEBVQTextOption@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalDrawText6),

        reinterpret_cast<void*>(HookedDrawText6),

        {

            {L"Qt5Gui.dll", "?drawText@QPainter@QT@@QEAAXAEBVQRectF@2@HAEBVQString@2@PEAV32@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@QT@@QEAAXAEBVQRectF@2@HAEBVQString@2@PEAV32@@Z"},

            {L"Qt5Gui.dll", "?drawText@QPainter@@QEAAXAEBVQRectF@@HAEBVQString@@PEAV2@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@@QEAAXAEBVQRectF@@HAEBVQString@@PEAV2@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQLabelSetText),

        reinterpret_cast<void*>(HookedQLabelSetText),

        {

            {L"Qt5Widgets.dll", "?setText@QLabel@QT@@QEAAXAEBVQString@2@@Z"},

            {L"Qt6Widgets.dll", "?setText@QLabel@QT@@QEAAXAEBVQString@2@@Z"},

            {L"Qt5Widgets.dll", "?setText@QLabel@@QEAAXAEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?setText@QLabel@@QEAAXAEBVQString@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQWidgetSetToolTip),

        reinterpret_cast<void*>(HookedQWidgetSetToolTip),

        {

            {L"Qt5Widgets.dll", "?setToolTip@QWidget@QT@@QEAAXAEBVQString@2@@Z"},

            {L"Qt6Widgets.dll", "?setToolTip@QWidget@QT@@QEAAXAEBVQString@2@@Z"},

            {L"Qt5Widgets.dll", "?setToolTip@QWidget@@QEAAXAEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?setToolTip@QWidget@@QEAAXAEBVQString@@@Z"},

        },

    },




    {

        reinterpret_cast<void**>(&OriginalQMainWindowSetCentralWidget),

        reinterpret_cast<void*>(HookedQMainWindowSetCentralWidget),

        {

            {L"Qt5Widgets.dll", "?setCentralWidget@QMainWindow@QT@@QEAAXPEAVQWidget@2@@Z"},

            {L"Qt6Widgets.dll", "?setCentralWidget@QMainWindow@QT@@QEAAXPEAVQWidget@2@@Z"},

            {L"Qt5Widgets.dll", "?setCentralWidget@QMainWindow@@QEAAXPEAVQWidget@@@Z"},

            {L"Qt6Widgets.dll", "?setCentralWidget@QMainWindow@@QEAAXPEAVQWidget@@@Z"},

        },

    },

    {
        reinterpret_cast<void**>(&OriginalQMainWindowSetMenuBar),

        reinterpret_cast<void*>(HookedQMainWindowSetMenuBar),

        {

            {L"Qt5Widgets.dll", "?setMenuBar@QMainWindow@QT@@QEAAXPEAVQMenuBar@2@@Z"},

            {L"Qt6Widgets.dll", "?setMenuBar@QMainWindow@QT@@QEAAXPEAVQMenuBar@2@@Z"},

            {L"Qt5Widgets.dll", "?setMenuBar@QMainWindow@@QEAAXPEAVQMenuBar@@@Z"},

            {L"Qt6Widgets.dll", "?setMenuBar@QMainWindow@@QEAAXPEAVQMenuBar@@@Z"},

        },

    },

    {
        reinterpret_cast<void**>(&OriginalQWidgetAddActionObject),

        reinterpret_cast<void*>(HookedQWidgetAddActionObject),

        {

            {L"Qt5Widgets.dll", "?addAction@QWidget@QT@@QEAAXPEAVQAction@2@@Z"},

            {L"Qt6Widgets.dll", "?addAction@QWidget@QT@@QEAAXPEAVQAction@2@@Z"},

            {L"Qt5Widgets.dll", "?addAction@QWidget@@QEAAXPEAVQAction@@@Z"},

            {L"Qt6Widgets.dll", "?addAction@QWidget@@QEAAXPEAVQAction@@@Z"},

        },

    },

    {
        reinterpret_cast<void**>(&OriginalQMenuBarAddMenu),

        reinterpret_cast<void*>(HookedQMenuBarAddMenu),

        {

            {L"Qt5Widgets.dll", "?addMenu@QMenuBar@QT@@QEAAPEAVQMenu@2@AEBVQString@2@@Z"},

            {L"Qt6Widgets.dll", "?addMenu@QMenuBar@QT@@QEAAPEAVQMenu@2@AEBVQString@2@@Z"},

            {L"Qt5Widgets.dll", "?addMenu@QMenuBar@@QEAAPEAVQMenu@@AEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?addMenu@QMenuBar@@QEAAPEAVQMenu@@AEBVQString@@@Z"},

        },

    },

    {


        reinterpret_cast<void**>(&OriginalQMenuBarAddMenuObject),

        reinterpret_cast<void*>(HookedQMenuBarAddMenuObject),

        {

            {L"Qt5Widgets.dll", "?addMenu@QMenuBar@QT@@QEAAPEAVQMenu@2@PEAV32@@Z"},

            {L"Qt6Widgets.dll", "?addMenu@QMenuBar@QT@@QEAAPEAVQMenu@2@PEAV32@@Z"},

            {L"Qt5Widgets.dll", "?addMenu@QMenuBar@@QEAAPEAVQAction@@PEAVQMenu@@@Z"},

            {L"Qt6Widgets.dll", "?addMenu@QMenuBar@@QEAAPEAVQAction@@PEAVQMenu@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQMenuSetTitle),

        reinterpret_cast<void*>(HookedQMenuSetTitle),

        {

            {L"Qt5Widgets.dll", "?setTitle@QMenu@QT@@QEAAXAEBVQString@2@@Z"},

            {L"Qt6Widgets.dll", "?setTitle@QMenu@QT@@QEAAXAEBVQString@2@@Z"},

            {L"Qt5Widgets.dll", "?setTitle@QMenu@@QEAAXAEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?setTitle@QMenu@@QEAAXAEBVQString@@@Z"},

        },

    },

    {
        reinterpret_cast<void**>(&OriginalQMenuBarAddAction),

        reinterpret_cast<void*>(HookedQMenuBarAddAction),

        {

            {L"Qt5Widgets.dll", "?addAction@QMenuBar@QT@@QEAAPEAVQAction@2@AEBVQString@2@@Z"},

            {L"Qt6Widgets.dll", "?addAction@QMenuBar@QT@@QEAAPEAVQAction@2@AEBVQString@2@@Z"},

            {L"Qt5Widgets.dll", "?addAction@QMenuBar@@QEAAPEAVQAction@@AEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?addAction@QMenuBar@@QEAAPEAVQAction@@AEBVQString@@@Z"},

        },

    },


    {

        reinterpret_cast<void**>(&OriginalQMenuBarAddActionObject),

        reinterpret_cast<void*>(HookedQMenuBarAddActionObject),

        {

            {L"Qt5Widgets.dll", "?addAction@QMenuBar@QT@@QEAAPEAVQAction@2@PEAV32@@Z"},

            {L"Qt6Widgets.dll", "?addAction@QMenuBar@QT@@QEAAPEAVQAction@2@PEAV32@@Z"},

            {L"Qt5Widgets.dll", "?addAction@QMenuBar@@QEAAPEAVQAction@@PEAV2@@Z"},

            {L"Qt6Widgets.dll", "?addAction@QMenuBar@@QEAAPEAVQAction@@PEAV2@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQActionActivate),

        reinterpret_cast<void*>(HookedQActionActivate),

        {

            {L"Qt5Widgets.dll", "?activate@QAction@@QEAAXW4ActionEvent@1@@Z"},

            {L"Qt6Widgets.dll", "?activate@QAction@@QEAAXW4ActionEvent@1@@Z"},

        },

    },

    {
        reinterpret_cast<void**>(&OriginalQActionTrigger),

        reinterpret_cast<void*>(HookedQActionTrigger),

        {

            {L"Qt5Widgets.dll", "?trigger@QAction@QT@@QEAAXXZ"},

            {L"Qt6Widgets.dll", "?trigger@QAction@QT@@QEAAXXZ"},

            {L"Qt5Widgets.dll", "?trigger@QAction@@QEAAXXZ"},

            {L"Qt6Widgets.dll", "?trigger@QAction@@QEAAXXZ"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQWindowSetTitle),

        reinterpret_cast<void*>(HookedQWindowSetTitle),

        {

            {L"Qt5Gui.dll", "?setTitle@QWindow@QT@@QEAAXAEBVQString@2@@Z"},

            {L"Qt6Gui.dll", "?setTitle@QWindow@QT@@QEAAXAEBVQString@2@@Z"},

            {L"Qt5Gui.dll", "?setTitle@QWindow@@QEAAXAEBVQString@@@Z"},

            {L"Qt6Gui.dll", "?setTitle@QWindow@@QEAAXAEBVQString@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQFileDialogGetOpenFileName),

        reinterpret_cast<void*>(HookedQFileDialogGetOpenFileName),

        {

			{L"Qt5Widgets.dll", "?getOpenFileName@QFileDialog@QT@@SA?AVQString@2@PEAVQWidget@2@AEBV32@11PEAV32@V?$QFlags@W4Option@QFileDialog@QT@@@2@@Z"},

            {L"Qt6Widgets.dll", "?getOpenFileName@QFileDialog@QT@@SA?AVQString@2@PEAVQWidget@2@AEBV32@11PEAV32@V?$QFlags@W4Option@QFileDialog@QT@@@2@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQFileDialogGetExistingDirectory),

        reinterpret_cast<void*>(HookedQFileDialogGetExistingDirectory),

        {

            {L"Qt5Widgets.dll", "?getExistingDirectory@QFileDialog@QT@@SA?AVQString@2@PEAVQWidget@2@AEBV32@1V?$QFlags@W4Option@QFileDialog@QT@@@2@@Z"},

            {L"Qt6Widgets.dll", "?getExistingDirectory@QFileDialog@QT@@SA?AVQString@2@PEAVQWidget@2@AEBV32@1V?$QFlags@W4Option@QFileDialog@QT@@@2@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQFontMetricsSize),

        reinterpret_cast<void*>(HookedFontMetricsSize),

        {

            {L"Qt5Gui.dll", "?size@QFontMetrics@@QEBA?AVQSize@@HAEBVQString@@HPEAH@Z"},

            {L"Qt6Gui.dll", "?size@QFontMetrics@@QEBA?AVQSize@@HAEBVQString@@HPEAH@Z"},

            {L"Qt6Gui.dll", "?size@QFontMetrics@QT@@QEBA?AVQSize@2@HAEBVQString@2@HPEAH@Z"},

            {L"Qt5Gui.dll", "?size@QFontMetrics@QT@@QEBA?AVQSize@2@HAEBVQString@2@HPEAH@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQAbstractButtonSetText),

        reinterpret_cast<void*>(HookedQAbstractButtonSetText),

        {

            {L"Qt5Widgets.dll", "?setText@QAbstractButton@@QEAAXAEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?setText@QAbstractButton@@QEAAXAEBVQString@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQActionSetText),

        reinterpret_cast<void*>(HookedQActionSetText),

        {

            {L"Qt5Widgets.dll", "?setText@QAction@@QEAAXAEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?setText@QAction@@QEAAXAEBVQString@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQGroupBoxSetTitle),

        reinterpret_cast<void*>(HookedQGroupBoxSetTitle),

        {

            {L"Qt5Widgets.dll", "?setTitle@QGroupBox@@QEAAXAEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?setTitle@QGroupBox@@QEAAXAEBVQString@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQWidgetSetWindowTitle),

        reinterpret_cast<void*>(HookedQWidgetSetWindowTitle),

        {

            {L"Qt5Widgets.dll", "?setWindowTitle@QWidget@@QEAAXAEBVQString@@@Z"},

            {L"Qt6Widgets.dll", "?setWindowTitle@QWidget@@QEAAXAEBVQString@@@Z"},

        },

    },

    {
        reinterpret_cast<void**>(&OriginalQComboBoxSetItemText),
        reinterpret_cast<void*>(HookedQComboBoxSetItemText),
        {
            {L"Qt5Widgets.dll", "?setItemText@QComboBox@@QEAAXHAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setItemText@QComboBox@@QEAAXHAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQToolBoxSetItemText),
        reinterpret_cast<void*>(HookedQToolBoxSetItemText),
        {
            {L"Qt5Widgets.dll", "?setItemText@QToolBox@@QEAAXHAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setItemText@QToolBox@@QEAAXHAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQTabBarSetTabText),
        reinterpret_cast<void*>(HookedQTabBarSetTabText),
        {
            {L"Qt5Widgets.dll", "?setTabText@QTabBar@@QEAAXHAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setTabText@QTabBar@@QEAAXHAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQTabWidgetSetTabText),
        reinterpret_cast<void*>(HookedQTabWidgetSetTabText),
        {
            {L"Qt5Widgets.dll", "?setTabText@QTabWidget@@QEAAXHAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setTabText@QTabWidget@@QEAAXHAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQComboBoxSetPlaceholderText),
        reinterpret_cast<void*>(HookedQComboBoxSetPlaceholderText),
        {
            {L"Qt5Widgets.dll", "?setPlaceholderText@QComboBox@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setPlaceholderText@QComboBox@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQLineEditSetPlaceholderText),
        reinterpret_cast<void*>(HookedQLineEditSetPlaceholderText),
        {
            {L"Qt5Widgets.dll", "?setPlaceholderText@QLineEdit@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setPlaceholderText@QLineEdit@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQPlainTextEditSetPlaceholderText),
        reinterpret_cast<void*>(HookedQPlainTextEditSetPlaceholderText),
        {
            {L"Qt5Widgets.dll", "?setPlaceholderText@QPlainTextEdit@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setPlaceholderText@QPlainTextEdit@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQTextEditSetPlaceholderText),
        reinterpret_cast<void*>(HookedQTextEditSetPlaceholderText),
        {
            {L"Qt5Widgets.dll", "?setPlaceholderText@QTextEdit@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setPlaceholderText@QTextEdit@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQTextDocumentSetHtml),
        reinterpret_cast<void*>(HookedQTextDocumentSetHtml),
        {
            {L"Qt5Gui.dll", "?setHtml@QTextDocument@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Gui.dll", "?setHtml@QTextDocument@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQTextDocumentSetPlainText),
        reinterpret_cast<void*>(HookedQTextDocumentSetPlainText),
        {
            {L"Qt5Gui.dll", "?setPlainText@QTextDocument@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Gui.dll", "?setPlainText@QTextDocument@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQTextEditSetHtml),
        reinterpret_cast<void*>(HookedQTextEditSetHtml),
        {
            {L"Qt5Widgets.dll", "?setHtml@QTextEdit@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setHtml@QTextEdit@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQTextEditInsertHtml),
        reinterpret_cast<void*>(HookedQTextEditInsertHtml),
        {
            {L"Qt5Widgets.dll", "?insertHtml@QTextEdit@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?insertHtml@QTextEdit@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQTextEditSetPlainText),
        reinterpret_cast<void*>(HookedQTextEditSetPlainText),
        {
            {L"Qt5Widgets.dll", "?setPlainText@QTextEdit@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setPlainText@QTextEdit@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQPlainTextEditSetPlainText),
        reinterpret_cast<void*>(HookedQPlainTextEditSetPlainText),
        {
            {L"Qt5Widgets.dll", "?setPlainText@QPlainTextEdit@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setPlainText@QPlainTextEdit@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQMessageBoxSetText),
        reinterpret_cast<void*>(HookedQMessageBoxSetText),
        {
            {L"Qt5Widgets.dll", "?setText@QMessageBox@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setText@QMessageBox@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQMessageBoxSetInformativeText),
        reinterpret_cast<void*>(HookedQMessageBoxSetInformativeText),
        {
            {L"Qt5Widgets.dll", "?setInformativeText@QMessageBox@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setInformativeText@QMessageBox@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQMessageBoxSetDetailedText),
        reinterpret_cast<void*>(HookedQMessageBoxSetDetailedText),
        {
            {L"Qt5Widgets.dll", "?setDetailedText@QMessageBox@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setDetailedText@QMessageBox@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQWidgetSetStatusTip),
        reinterpret_cast<void*>(HookedQWidgetSetStatusTip),
        {
            {L"Qt5Widgets.dll", "?setStatusTip@QWidget@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setStatusTip@QWidget@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQActionSetStatusTip),
        reinterpret_cast<void*>(HookedQActionSetStatusTip),
        {
            {L"Qt5Widgets.dll", "?setStatusTip@QAction@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setStatusTip@QAction@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQActionSetToolTip),
        reinterpret_cast<void*>(HookedQActionSetToolTip),
        {
            {L"Qt5Widgets.dll", "?setToolTip@QAction@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setToolTip@QAction@@QEAAXAEBVQString@@@Z"},
        },
    },

    {
        reinterpret_cast<void**>(&OriginalQActionSetWhatsThis),
        reinterpret_cast<void*>(HookedQActionSetWhatsThis),
        {
            {L"Qt5Widgets.dll", "?setWhatsThis@QAction@@QEAAXAEBVQString@@@Z"},
            {L"Qt6Widgets.dll", "?setWhatsThis@QAction@@QEAAXAEBVQString@@@Z"},
        },
    },

    {

        reinterpret_cast<void**>(&OriginalQFontMetricsElidedText),

        reinterpret_cast<void*>(HookedFontMetricsElidedText),

        {

            {L"Qt5Gui.dll", "?elidedText@QFontMetrics@@QEBA?AVQString@@AEBV2@W4TextElideMode@Qt@@HH@Z"},

            {L"Qt6Gui.dll", "?elidedText@QFontMetrics@@QEBA?AVQString@@AEBV2@W4TextElideMode@Qt@@HH@Z"},

            {L"Qt5Gui.dll", "?elidedText@QFontMetrics@QT@@QEBA?AVQString@2@AEBV32@W4TextElideMode@Qt@@HH@Z"},

            {L"Qt6Gui.dll", "?elidedText@QFontMetrics@QT@@QEBA?AVQString@2@AEBV32@W4TextElideMode@Qt@@HH@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalQFontMetricsFElidedText),

        reinterpret_cast<void*>(HookedFontMetricsFElidedText),

        {

            {L"Qt5Gui.dll", "?elidedText@QFontMetricsF@@QEBA?AVQString@@AEBV2@W4TextElideMode@Qt@@NH@Z"},

            {L"Qt6Gui.dll", "?elidedText@QFontMetricsF@@QEBA?AVQString@@AEBV2@W4TextElideMode@Qt@@NH@Z"},

            {L"Qt5Gui.dll", "?elidedText@QFontMetricsF@QT@@QEBA?AVQString@2@AEBV32@W4TextElideMode@Qt@@NH@Z"},

            {L"Qt6Gui.dll", "?elidedText@QFontMetricsF@QT@@QEBA?AVQString@2@AEBV32@W4TextElideMode@Qt@@NH@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalDrawText7),

        reinterpret_cast<void*>(HookedDrawText7),

        {

            {L"Qt5Gui.dll", "?drawText@QPainter@@QEAAXHHHHHAEBVQString@@PEAVQRect@@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@@QEAAXHHHHHAEBVQString@@PEAVQRect@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalDrawTextPoint),

        reinterpret_cast<void*>(HookedDrawTextPoint),

        {

            {L"Qt5Gui.dll", "?drawText@QPainter@@QEAAXAEBVQPoint@@AEBVQString@@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@@QEAAXAEBVQPoint@@AEBVQString@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalDrawTextPointF),

        reinterpret_cast<void*>(HookedDrawTextPointF),

        {

            {L"Qt5Gui.dll", "?drawText@QPainter@@QEAAXAEBVQPointF@@AEBVQString@@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@@QEAAXAEBVQPointF@@AEBVQString@@@Z"},

        },

    },

    {

        reinterpret_cast<void**>(&OriginalDrawTextXY),

        reinterpret_cast<void*>(HookedDrawTextXY),

        {

            {L"Qt5Gui.dll", "?drawText@QPainter@@QEAAXHHAEBVQString@@@Z"},

            {L"Qt6Gui.dll", "?drawText@QPainter@@QEAAXHHAEBVQString@@@Z"},

        },

    },

};



bool Hook::Initialize() {

    const std::wstring qtCoreModuleName = DetectQtCoreModuleName();

    std::size_t registeredHookCount = 0;

    bool qtInitialized = false;



    TranslationManagerConfig translationConfig;

    translationConfig.filterChineseSourceWrites = true;

    TranslationManager::Configure(translationConfig);

    if (!TranslationManager::Initialize(L"Dictionaries/translations.txt")) {

        Logger::Write(L"[Hook] Failed to load Dictionaries/translations.txt");

        return false;

    }



    if (!qtCoreModuleName.empty()) {

        Logger::Write(L"[Hook] selected QtCore module: %s", qtCoreModuleName.c_str());

        for (const HookFunctionInfo& hook : kHookFunctions) {

            if (RegisterHookFunction(hook, qtCoreModuleName)) {

                ++registeredHookCount;

            }

        }



        if (registeredHookCount > 0) {

            QStringBridge::Config qstringBridgeConfig;

            qstringBridgeConfig.module_name = qtCoreModuleName;

            qstringBridgeConfig.load_if_missing = false;

            qtInitialized = qstringBridge_.Initialize(qstringBridgeConfig);

            if (!qtInitialized) {

                Logger::Write(L"[Hook] QStringBridge initialization failed; continuing with Win32/GDI hooks");

                detoursHook_.Release();

                registeredHookCount = 0;

            }

        }

    } else {

        Logger::Write(L"[Hook] QtCore not found; continuing with Win32/GDI hooks");

    }



#if defined(XPROXY_KNALD_ONLY) || defined(XPROXY_WORLDMACHINE_ONLY)
    constexpr bool gdiInitialized = false;
#else
    const bool gdiInitialized = Win32GdiHook::Initialize();

    if (gdiInitialized) {

        ++registeredHookCount;

    }
#endif

    if (qtInitialized && !detoursHook_.Commit()) {

        Logger::Write(L"[Hook] Qt Detours commit failed");

        detoursHook_.Release();

        qstringBridge_.Reset();

        qtInitialized = false;

        --registeredHookCount;

    }



    if (registeredHookCount == 0) {

        Logger::Write(L"[Hook] no hook was installed");

        Win32GdiHook::Uninitialize();

        qstringBridge_.Reset();

        TranslationManager::Clear();

        return false;

    }



    Logger::Write(L"[Hook] hooks installed. qt=%d win32_gdi=%d", qtInitialized ? 1 : 0, gdiInitialized ? 1 : 0);

    return true;

}

void Hook::RefreshTranslations() {
    RefreshMenuActions();
}

void Hook::Uninitialize() {

    Win32GdiHook::Uninitialize();

    detoursHook_.Release();



    OriginalDrawText3 = nullptr;

    OriginalDrawText4 = nullptr;

    OriginalDrawText5 = nullptr;

    OriginalDrawText6 = nullptr;

    OriginalDrawText7 = nullptr;

    OriginalDrawTextPoint = nullptr;

    OriginalDrawTextPointF = nullptr;

    OriginalDrawTextXY = nullptr;

    OriginalQLabelSetText = nullptr;

    OriginalQAbstractButtonSetText = nullptr;
    OriginalQActionSetText = nullptr;
    OriginalQGroupBoxSetTitle = nullptr;
    OriginalQWidgetSetWindowTitle = nullptr;
    OriginalQComboBoxSetItemText = nullptr;
    OriginalQToolBoxSetItemText = nullptr;
    OriginalQTabBarSetTabText = nullptr;
    OriginalQTabWidgetSetTabText = nullptr;
    OriginalQComboBoxSetPlaceholderText = nullptr;
    OriginalQLineEditSetPlaceholderText = nullptr;
    OriginalQPlainTextEditSetPlaceholderText = nullptr;
    OriginalQTextEditSetPlaceholderText = nullptr;
    OriginalQTextDocumentSetHtml = nullptr;
    OriginalQTextDocumentSetPlainText = nullptr;
    OriginalQTextEditSetHtml = nullptr;
    OriginalQTextEditInsertHtml = nullptr;
    OriginalQTextEditSetPlainText = nullptr;
    OriginalQPlainTextEditSetPlainText = nullptr;
    OriginalQMessageBoxSetText = nullptr;
    OriginalQMessageBoxSetInformativeText = nullptr;
    OriginalQMessageBoxSetDetailedText = nullptr;
    OriginalQWidgetSetStatusTip = nullptr;
    OriginalQActionSetStatusTip = nullptr;
    OriginalQActionSetToolTip = nullptr;
    OriginalQActionSetWhatsThis = nullptr;

    OriginalQWidgetSetToolTip = nullptr;
    OriginalQMainWindowSetMenuBar = nullptr;
    OriginalQMainWindowSetCentralWidget = nullptr;
    OriginalQWidgetAddActionObject = nullptr;
    OriginalQWidgetRemoveAction = nullptr;

    OriginalQMenuBarAddMenu = nullptr;

    OriginalQMenuBarAddMenuObject = nullptr;

    OriginalQMenuSetTitle = nullptr;
    OriginalQMenuTitle = nullptr;

    OriginalQMenuBarAddAction = nullptr;
    OriginalQMenuBarAddActionObject = nullptr;
    OriginalQActionText = nullptr;

    OriginalQActionActivate = nullptr;
    OriginalQActionTrigger = nullptr;

    g_languageAction = nullptr;

    g_helpMenu = nullptr;
    g_menuBar = nullptr;

    OriginalQWindowSetTitle = nullptr;

    OriginalQFileDialogGetOpenFileName = nullptr;

    OriginalQFileDialogGetExistingDirectory = nullptr;

    OriginalQFontMetricsSize = nullptr;

    OriginalQFontMetricsElidedText = nullptr;

    OriginalQFontMetricsFElidedText = nullptr;

    OriginalQFontMetricsElidedText = nullptr;

    qstringBridge_.Reset();

    TranslationManager::Clear();

    Logger::Write(L"[Hook] 已卸载");

}





void HookedDrawText3(void* pThis, void* pointF, const void* text, int textFlags, int justificationPadding) {

    try {

        auto translated = QStringTranslation(text, "drawText-point");

        OriginalDrawText3(pThis, pointF, translated ? translated.Get() : text, textFlags, justificationPadding);

    } catch (...) {

        OriginalDrawText3(pThis, pointF, text, textFlags, justificationPadding);

    }

}



void HookedDrawText4(void* pThis, void* rect, int flags, const void* text, void* boundingRect) {

    try {

        auto translated = QStringTranslation(text, "drawText-rect");

        OriginalDrawText4(pThis, rect, flags, translated ? translated.Get() : text, boundingRect);

    } catch (...) {

        OriginalDrawText4(pThis, rect, flags, text, boundingRect);

    }

}



void HookedDrawText5(void* pThis, void* rectangle, const void* text, void* option) {

    try {

        auto translated = QStringTranslation(text, "drawText-rectf-option");

        OriginalDrawText5(pThis, rectangle, translated ? translated.Get() : text, option);

    } catch (...) {

        OriginalDrawText5(pThis, rectangle, text, option);

    }

}



void HookedDrawText6(void* pThis, void* rectangle, int flags, const void* text, void* boundingRect) {

    try {

        auto translated = QStringTranslation(text, "drawText-rectf-flags");

        OriginalDrawText6(pThis, rectangle, flags, translated ? translated.Get() : text, boundingRect);

    } catch (...) {

        OriginalDrawText6(pThis, rectangle, flags, text, boundingRect);

    }

}



void HookedDrawText7(

    void* pThis,

    int x,

    int y,

    int width,

    int height,

    int flags,

    const void* text,

    void* boundingRect

) {

    try {

        auto translated = QStringTranslation(text, "drawText-xywh");

        OriginalDrawText7(

            pThis,

            x,

            y,

            width,

            height,

            flags,

            translated ? translated.Get() : text,

            boundingRect

        );

    } catch (...) {

        OriginalDrawText7(pThis, x, y, width, height, flags, text, boundingRect);

    }

}



void HookedDrawTextPoint(void* pThis, const void* point, const void* text) {

    try {

        auto translated = QStringTranslation(text, "drawText-point-int");

        OriginalDrawTextPoint(pThis, point, translated ? translated.Get() : text);

    } catch (...) {

        OriginalDrawTextPoint(pThis, point, text);

    }

}



void HookedDrawTextPointF(void* pThis, const void* point, const void* text) {

    try {

        auto translated = QStringTranslation(text, "drawText-pointf");

        OriginalDrawTextPointF(pThis, point, translated ? translated.Get() : text);

    } catch (...) {

        OriginalDrawTextPointF(pThis, point, text);

    }

}



void HookedDrawTextXY(void* pThis, int x, int y, const void* text) {

    try {

        auto translated = QStringTranslation(text, "drawText-xy");

        OriginalDrawTextXY(pThis, x, y, translated ? translated.Get() : text);

    } catch (...) {

        OriginalDrawTextXY(pThis, x, y, text);

    }

}



void HookedQLabelSetText(void* pThis, const void* text) {

    try {

        auto translated = QStringTranslation(text, "label-setText");

        OriginalQLabelSetText(pThis, translated ? translated.Get() : text);

    } catch (...) {

        OriginalQLabelSetText(pThis, text);

    }

}



void HookedQMainWindowSetCentralWidget(void* pThis, void* widget) {
    OriginalQMainWindowSetCentralWidget(pThis, widget);
    if (g_menuBar != nullptr && g_languageAction != nullptr) {
        MoveLanguageActionAfterHelp(g_menuBar);
    }
}


void HookedQMainWindowSetMenuBar(void* pThis, void* menuBar) {
    g_menuBar = menuBar;
    OriginalQMainWindowSetMenuBar(pThis, menuBar);
    if (menuBar != nullptr) {
        AddLanguageAction(menuBar);
    }
}


void HookedQWidgetAddActionObject(void* pThis, void* action) {
    OriginalQWidgetAddActionObject(pThis, action);
    if (IsHelpActionObject(action)) {
        if (g_languageAction != nullptr) {
            MoveLanguageActionAfterHelp(g_menuBar != nullptr ? g_menuBar : pThis);
        } else {
            AddLanguageAction(pThis);
        }
    }
}


void HookedQWidgetSetToolTip(void* pThis, const void* text) {

    try {

        auto translated = QStringTranslation(text, "widget-setToolTip");

        OriginalQWidgetSetToolTip(pThis, translated ? translated.Get() : text);

    } catch (...) {

        OriginalQWidgetSetToolTip(pThis, text);

    }

}


static bool IsHelpMenuText(const void* text) {

    int length = 0;
    const wchar_t* value = qstringBridge_.Extract(text, length);
    if (value == nullptr || length <= 0) {
        return false;
    }

    std::wstring label(value, value + length);
    label.erase(std::remove(label.begin(), label.end(), L'&'), label.end());
    while (!label.empty() && iswspace(label.front()) != 0) {
        label.erase(label.begin());
    }
    while (!label.empty() && iswspace(label.back()) != 0) {
        label.pop_back();
    }
    return _wcsicmp(label.c_str(), L"Help") == 0 || label == L"帮助";
}

static void AddLanguageAction(void* menuBar) {
#if !defined(XPROXY_KNALD_ONLY)
    (void)menuBar;
#else
    if (g_languageAction != nullptr || OriginalQMenuBarAddAction == nullptr ||
        !qstringBridge_.CanCreate()) {
        return;
    }

    auto label = qstringBridge_.CreateScoped(kLanguageActionText);
    if (label) {
        g_languageAction = OriginalQMenuBarAddAction(menuBar, label.Get());
    }
#endif
}


static void MoveLanguageActionAfterHelp(void* menuBar) {
    if (menuBar == nullptr || g_languageAction == nullptr || OriginalQWidgetAddActionObject == nullptr) {
        return;
    }
    if (OriginalQWidgetRemoveAction == nullptr) {
        HMODULE module = GetModuleHandleW(L"Qt5Widgets.dll");
        if (module == nullptr) {
            module = GetModuleHandleW(L"Qt6Widgets.dll");
        }
        if (module != nullptr) {
            constexpr const char* removeActionName = "?removeAction@QWidget@@QEAAXPEAVQAction@@@Z";
            OriginalQWidgetRemoveAction = reinterpret_cast<QWidgetRemoveAction_t>(
                GetProcAddress(module, removeActionName));
        }
    }
    if (OriginalQWidgetRemoveAction == nullptr) {
        return;
    }
    void* action = g_languageAction;
    OriginalQWidgetRemoveAction(menuBar, action);
    OriginalQWidgetAddActionObject(menuBar, action);
}

void* HookedQMenuBarAddMenu(void* pThis, const void* title) {
    void* menu = OriginalQMenuBarAddMenu(pThis, title);
    if (IsHelpMenuText(title)) {
        if (g_languageAction != nullptr) {
            MoveLanguageActionAfterHelp(g_menuBar != nullptr ? g_menuBar : pThis);
        } else {
            AddLanguageAction(pThis);
        }
    }
    return menu;
}

static void ResolveQtMenuReaderFunctions() {
    HMODULE module = GetModuleHandleW(L"Qt5Widgets.dll");
    if (module == nullptr) {
        module = GetModuleHandleW(L"Qt6Widgets.dll");
    }
    if (module == nullptr) {
        return;
    }
    if (OriginalQMenuTitle == nullptr) {
        OriginalQMenuTitle = reinterpret_cast<QMenuTitle_t>(
            GetProcAddress(module, "?title@QMenu@@QEBA?AVQString@@XZ"));
    }
    if (OriginalQActionText == nullptr) {
        OriginalQActionText = reinterpret_cast<QActionText_t>(
            GetProcAddress(module, "?text@QAction@@QEBA?AVQString@@XZ"));
    }
    if (OriginalQWidgetActions == nullptr) {
        OriginalQWidgetActions = reinterpret_cast<QWidgetActions_t>(
            GetProcAddress(module, "?actions@QWidget@@QEBA?AV?$QList@PEAVQAction@@@@XZ"));
    }
    if (OriginalQActionMenu == nullptr) {
        OriginalQActionMenu = reinterpret_cast<QActionMenu_t>(
            GetProcAddress(module, "?menu@QAction@@QEBAPEAVQMenu@@XZ"));
    }
}


static bool IsHelpMenuObject(void* menu) {
    ResolveQtMenuReaderFunctions();
    if (menu == nullptr || OriginalQMenuTitle == nullptr || !qstringBridge_.CanCreate()) {
        return false;
    }
    auto title = qstringBridge_.CreateScoped(L"");
    if (!title) {
        return false;
    }
    OriginalQMenuTitle(title.Get(), menu);
    return IsHelpMenuText(title.Get());
}

void* HookedQMenuBarAddMenuObject(void* pThis, void* menu) {
    void* result = OriginalQMenuBarAddMenuObject(pThis, menu);
    if (menu == g_helpMenu || IsHelpMenuObject(menu)) {
        if (g_languageAction != nullptr) {
            MoveLanguageActionAfterHelp(g_menuBar != nullptr ? g_menuBar : pThis);
        } else {
            AddLanguageAction(pThis);
        }
    }
    return result;
}

void HookedQMenuSetTitle(void* pThis, const void* title) {
    if (IsHelpMenuText(title)) {
        g_helpMenu = pThis;
    } else if (g_helpMenu == pThis) {
        g_helpMenu = nullptr;
    }
    OriginalQMenuSetTitle(pThis, title);
}


void* HookedQMenuBarAddAction(void* pThis, const void* text) {
    void* action = OriginalQMenuBarAddAction(pThis, text);
    if (IsHelpMenuText(text)) {
        if (g_languageAction != nullptr) {
            MoveLanguageActionAfterHelp(g_menuBar != nullptr ? g_menuBar : pThis);
        } else {
            AddLanguageAction(pThis);
        }
    }
    return action;
}

static bool IsHelpActionObject(void* action) {
    ResolveQtMenuReaderFunctions();
    if (action == nullptr || OriginalQActionText == nullptr || !qstringBridge_.CanCreate()) {
        return false;
    }
    auto text = qstringBridge_.CreateScoped(L"");
    if (!text) {
        return false;
    }
    OriginalQActionText(text.Get(), action);
    return IsHelpMenuText(text.Get());
}

void* HookedQMenuBarAddActionObject(void* pThis, void* action) {
    void* result = OriginalQMenuBarAddActionObject(pThis, action);
    if (IsHelpActionObject(action)) {
        if (g_languageAction != nullptr) {
            MoveLanguageActionAfterHelp(g_menuBar != nullptr ? g_menuBar : pThis);
        } else {
            AddLanguageAction(pThis);
        }
    }
    return result;
}


struct Qt5QListData {
    int referenceCount;
    int allocation;
    int begin;
    int end;
    void* array[1];
};

struct Qt5QList {
    Qt5QListData* data = nullptr;
};

static void RefreshMenuWidgetActions(
    void* widget,
    std::unordered_set<void*>& visitedWidgets,
    std::unordered_set<void*>& visitedActions
) {
    if (widget == nullptr || OriginalQWidgetActions == nullptr ||
        OriginalQActionText == nullptr || OriginalQActionSetText == nullptr ||
        !visitedWidgets.insert(widget).second) {
        return;
    }

    Qt5QList actions;
    OriginalQWidgetActions(&actions, widget);
    if (actions.data == nullptr || actions.data->end < actions.data->begin ||
        actions.data->end - actions.data->begin > 4096) {
        return;
    }

    auto** values = reinterpret_cast<void**>(actions.data->array);
    for (int index = actions.data->begin; index < actions.data->end; ++index) {
        void* action = values[index];
        if (action == nullptr || !visitedActions.insert(action).second) {
            continue;
        }

        auto currentText = qstringBridge_.CreateScoped(L"");
        if (currentText) {
            OriginalQActionText(currentText.Get(), action);
            int length = 0;
            const wchar_t* currentValue = qstringBridge_.Extract(currentText.Get(), length);
            if (currentValue != nullptr && length > 0) {
                const std::wstring current(currentValue, currentValue + length);
                const std::wstring_view target = TranslationManager::Translate(current, false);
                if (target != current) {
                    auto translated = qstringBridge_.CreateScoped(target);
                    if (translated) {
                        OriginalQActionSetText(action, translated.Get());
                    }
                }
            }
        }

        if (OriginalQActionMenu != nullptr) {
            RefreshMenuWidgetActions(OriginalQActionMenu(action), visitedWidgets, visitedActions);
        }
    }
}

void RefreshMenuActions() {
    ResolveQtMenuReaderFunctions();
    if (g_menuBar == nullptr || OriginalQWidgetActions == nullptr) {
        return;
    }

    std::unordered_set<void*> visitedWidgets;
    std::unordered_set<void*> visitedActions;
    RefreshMenuWidgetActions(g_menuBar, visitedWidgets, visitedActions);
}

void HookedQActionActivate(void* pThis, int event) {
#if defined(XPROXY_KNALD_ONLY)
    if (pThis == g_languageAction && event == 0) {
        TranslationManager::SetEnabled(!TranslationManager::IsEnabled());
        RefreshMenuActions();
        RedrawWindow(nullptr, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
#endif
    OriginalQActionActivate(pThis, event);
}


void HookedQActionTrigger(void* pThis) {
    OriginalQActionTrigger(pThis);
}

void HookedQWindowSetTitle(void* pThis, const void* text) {

    try {

        auto translated = QStringTranslation(text, "window-setTitle");

        OriginalQWindowSetTitle(pThis, translated ? translated.Get() : text);

    } catch (...) {

        OriginalQWindowSetTitle(pThis, text);

    }

}



void* HookedQFileDialogGetOpenFileName(

    void* outQString,

	void* parent,

	const void* caption,

	const void* dir,

	const void* filter,

	void* selectedFilter,

	unsigned int options) {

    try {

        auto translatedCaption = QStringTranslation(caption, "file-open-caption");

        auto translatedFilter = QStringTranslation(filter, "file-open-filter");

        return OriginalQFileDialogGetOpenFileName(

            outQString,

            parent,

            translatedCaption ? translatedCaption.Get() : caption,

            dir,

            translatedFilter ? translatedFilter.Get() : filter,

            selectedFilter,

            options

        );

    } catch (...) {

        return OriginalQFileDialogGetOpenFileName(outQString, parent, caption, dir, filter, selectedFilter, options);

    }

}



void* HookedQFileDialogGetExistingDirectory(

	void* outQString,

	void* parent,

	const void* caption,

	const void* dir,

	unsigned int options) {

    try {

        auto translatedCaption = QStringTranslation(caption, "directory-caption");

        return OriginalQFileDialogGetExistingDirectory(

            outQString,

            parent,

            translatedCaption ? translatedCaption.Get() : caption,

            dir,

            options

        );

    } catch (...) {

        return OriginalQFileDialogGetExistingDirectory(outQString, parent, caption, dir, options);

    }

}



void HookedQTextDocumentSetHtml(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "text-document-setHtml");
        OriginalQTextDocumentSetHtml(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQTextDocumentSetHtml(pThis, text);
    }
}


void HookedQTextDocumentSetPlainText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "text-document-setPlainText");
        OriginalQTextDocumentSetPlainText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQTextDocumentSetPlainText(pThis, text);
    }
}


void HookedQTextEditSetHtml(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "text-edit-setHtml");
        OriginalQTextEditSetHtml(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQTextEditSetHtml(pThis, text);
    }
}


void HookedQTextEditInsertHtml(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "text-edit-insertHtml");
        OriginalQTextEditInsertHtml(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQTextEditInsertHtml(pThis, text);
    }
}


void HookedQTextEditSetPlainText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "text-edit-setPlainText");
        OriginalQTextEditSetPlainText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQTextEditSetPlainText(pThis, text);
    }
}


void HookedQPlainTextEditSetPlainText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "plain-text-edit-setPlainText");
        OriginalQPlainTextEditSetPlainText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQPlainTextEditSetPlainText(pThis, text);
    }
}


void HookedQMessageBoxSetText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "message-box-setText");
        OriginalQMessageBoxSetText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQMessageBoxSetText(pThis, text);
    }
}


void HookedQMessageBoxSetInformativeText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "message-box-setInformativeText");
        OriginalQMessageBoxSetInformativeText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQMessageBoxSetInformativeText(pThis, text);
    }
}


void HookedQMessageBoxSetDetailedText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "message-box-setDetailedText");
        OriginalQMessageBoxSetDetailedText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQMessageBoxSetDetailedText(pThis, text);
    }
}


void HookedQWidgetSetStatusTip(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "widget-setStatusTip");
        OriginalQWidgetSetStatusTip(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQWidgetSetStatusTip(pThis, text);
    }
}


void HookedQActionSetStatusTip(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "action-setStatusTip");
        OriginalQActionSetStatusTip(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQActionSetStatusTip(pThis, text);
    }
}


void HookedQActionSetToolTip(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "action-setToolTip");
        OriginalQActionSetToolTip(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQActionSetToolTip(pThis, text);
    }
}


void HookedQActionSetWhatsThis(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "action-setWhatsThis");
        OriginalQActionSetWhatsThis(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQActionSetWhatsThis(pThis, text);
    }
}


void* HookedFontMetricsSize(void* pThis, void* result, int flags, void* text, int tabStops, int* tabArray) {
    try {
        auto translated = QStringTranslation(text, "font-metrics-size", false);
        return OriginalQFontMetricsSize(
            pThis,
            result,
            flags,
            const_cast<void*>(translated ? translated.Get() : text),
            tabStops,
            tabArray
        );
    } catch (...) {
        return OriginalQFontMetricsSize(pThis, result, flags, text, tabStops, tabArray);
    }
}


void HookedQAbstractButtonSetText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "abstract-button-setText");
        OriginalQAbstractButtonSetText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQAbstractButtonSetText(pThis, text);
    }
}


void HookedQActionSetText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "action-setText");
        OriginalQActionSetText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQActionSetText(pThis, text);
    }
}


void HookedQGroupBoxSetTitle(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "group-box-setTitle");
        OriginalQGroupBoxSetTitle(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQGroupBoxSetTitle(pThis, text);
    }
}


void HookedQWidgetSetWindowTitle(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "widget-setWindowTitle");
        OriginalQWidgetSetWindowTitle(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQWidgetSetWindowTitle(pThis, text);
    }
}


void HookedQComboBoxSetItemText(void* pThis, int index, const void* text) {
    try {
        auto translated = QStringTranslation(text, "combo-box-setItemText");
        OriginalQComboBoxSetItemText(pThis, index, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQComboBoxSetItemText(pThis, index, text);
    }
}


void HookedQToolBoxSetItemText(void* pThis, int index, const void* text) {
    try {
        auto translated = QStringTranslation(text, "tool-box-setItemText");
        OriginalQToolBoxSetItemText(pThis, index, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQToolBoxSetItemText(pThis, index, text);
    }
}


void HookedQTabBarSetTabText(void* pThis, int index, const void* text) {
    try {
        auto translated = QStringTranslation(text, "tab-bar-setTabText");
        OriginalQTabBarSetTabText(pThis, index, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQTabBarSetTabText(pThis, index, text);
    }
}


void HookedQTabWidgetSetTabText(void* pThis, int index, const void* text) {
    try {
        auto translated = QStringTranslation(text, "tab-widget-setTabText");
        OriginalQTabWidgetSetTabText(pThis, index, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQTabWidgetSetTabText(pThis, index, text);
    }
}


void HookedQComboBoxSetPlaceholderText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "combo-box-setPlaceholderText");
        OriginalQComboBoxSetPlaceholderText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQComboBoxSetPlaceholderText(pThis, text);
    }
}


void HookedQLineEditSetPlaceholderText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "line-edit-setPlaceholderText");
        OriginalQLineEditSetPlaceholderText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQLineEditSetPlaceholderText(pThis, text);
    }
}


void HookedQPlainTextEditSetPlaceholderText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "plain-text-edit-setPlaceholderText");
        OriginalQPlainTextEditSetPlaceholderText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQPlainTextEditSetPlaceholderText(pThis, text);
    }
}


void HookedQTextEditSetPlaceholderText(void* pThis, const void* text) {
    try {
        auto translated = QStringTranslation(text, "text-edit-setPlaceholderText");
        OriginalQTextEditSetPlaceholderText(pThis, translated ? translated.Get() : text);
    } catch (...) {
        OriginalQTextEditSetPlaceholderText(pThis, text);
    }
}


void* HookedFontMetricsElidedText(void* result, void* pThis, const void* text, int mode, int width, int flags) {
    try {
        auto translated = QStringTranslation(text, "font-metrics-elidedText", false);
        return OriginalQFontMetricsElidedText(
            result,
            pThis,
            translated ? translated.Get() : text,
            mode,
            width,
            flags
        );
    } catch (...) {
        return OriginalQFontMetricsElidedText(result, pThis, text, mode, width, flags);
    }
}


void* HookedFontMetricsFElidedText(void* result, void* pThis, const void* text, int mode, double width, int flags) {
    try {
        auto translated = QStringTranslation(text, "font-metrics-f-elidedText", false);
        return OriginalQFontMetricsFElidedText(
            result,
            pThis,
            translated ? translated.Get() : text,
            mode,
            width,
            flags
        );
    } catch (...) {
        return OriginalQFontMetricsFElidedText(result, pThis, text, mode, width, flags);
    }
}


static QStringBridge::ScopedQString QStringTranslation(

    const void* text,

    const char* hookName,

    bool writeUntranslated

) {

    (void)hookName;

    (void)writeUntranslated;



    if (text == nullptr) {

        return {};

    }



    int length = 0;

    const wchar_t* value = qstringBridge_.Extract(text, length);

    if (value == nullptr || length <= 0) {

        return {};

    }



    const std::wstring_view source(value, static_cast<std::size_t>(length));



    const std::wstring_view translated =

        TranslationManager::Translate(source, writeUntranslated);







    return qstringBridge_.CreateScoped(translated);

}

