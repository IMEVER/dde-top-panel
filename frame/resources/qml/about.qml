import QtQuick 2.9
import QtQuick.Window 2.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Window {
    visible: true
    width: 640
    height: 480
    title: "关于 " + appInfo.appName
    flags: Qt.WindowStaysOnTopHint | Qt.WindowMinimizeButtonHint | Qt.WindowCloseButtonHint
    screen: Qt.application.screens[0]
    x: screen.virtualX + ((screen.width - width) / 2)
    y: screen.virtualY + ((screen.height - height) / 2)

    signal closed

    onVisibilityChanged: {
        if (visibility==Window.Hidden)
            closed()
    }

  TabView {
      id: frame
      anchors.fill: parent
      anchors.margins: 4
      frameVisible: true
      Tab {
          title: "应用状态"

            GridLayout {
                columns: 2
                // anchors.fill: parent
                Layout.margins: 5
                    Text{
                        text: "名称"
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.minimumWidth: 100
                        Layout.preferredHeight: 30
                        Layout.row: 0
                        Layout.column: 0
                    }
                    Text {
                        Layout.row: 0
                        Layout.column: 1
                        Layout.preferredHeight: 30
                        text: appInfo.title
                    }
                    Text{
                        text: "文件"
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.preferredHeight: 30
                        Layout.row: 1
                        Layout.column: 0
                    }
                    Text {
                        Layout.row: 1
                        Layout.column: 1
                        Layout.preferredHeight: 30
                        text: appInfo.desktopFile
                    }
                    Text{
                        text: "机器"
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.row: 2
                        Layout.column: 0
                    }
                    Text {
                        Layout.row: 2
                        Layout.column: 1
                        text: appInfo.machine
                    }
                    Text{
                        text: "程序"
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.row: 3
                        Layout.column: 0
                    }
                    Text {
                        Layout.row: 3
                        Layout.column: 1
                        text: appInfo.cmdline
                    }
                    Text{
                        text: "类名"
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.row: 4
                        Layout.column: 0
                    }
                    Text {
                        Layout.row: 4
                        Layout.column: 1
                        text: appInfo.className
                    }
                    Text{
                        text: "桌面"
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.row: 5
                        Layout.column: 0
                    }
                    Text {
                        Layout.row: 5
                        Layout.column: 1
                        text: appInfo.desktop
                    }
                    Text{
                        text: "进程"
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.row: 6
                        Layout.column: 0
                    }
                    Text {
                        Layout.row: 6
                        Layout.column: 1
                        text: appInfo.pid
                    }
            }
      }
      Tab {
           title: "包信息"
          TableView {
                id: packageInfo
                anchors.fill: parent
                alternatingRowColors: true
                headerVisible: false

                function translate(field)
                {
                    if(field == "Package")
                        return "包名";
                    else if(field == "Status")
                        return "状态";
                    else if(field == "Priority")
                        return "优先级";
                    else if(field == "Section")
                        return "分类";
                    else if(field == "Installed-Size")
                        return "安装大小";
                    else if(field == "Maintainer")
                        return "维护者";
                    else if(field == "Architecture")
                        return "架构";
                    else if(field == "Version")
                        return "版本";
                    else if(field == "Depends")
                        return "依赖";
                    else if(field == "Description")
                        return "描述";
                    else if(field == "desc")
                        return "详细描述";
                    else if(field == "Homepage")
                        return "主页";
                    else if(field == "Replaces")
                        return "代替";
                    else if(field == "Provides")
                        return "提供";
                    else if(field == "Conflicts")
                        return "冲突";
                    else if(field == "Recommends")
                        return "推荐";
                    else if(field == "Breaks")
                        return "破坏";
                    else if(field == "Suggests")
                        return "建议";
                    else if(field == "Source")
                        return "源码";
                    else if(field == "Multi-Arch")
                        return "多架构";
                    else
                        return field;
                }

                TableViewColumn {
                   role: "name"
                   title: "字段"
                   width: 100
                //    horizontalAlignment: Text.AlignHCenter
                }
                TableViewColumn {
                    role: "value"
                    title: "值"
                    width: 530
                    elideMode: Text.ElideNone
                }
                model: appInfo.package
                rowDelegate: Rectangle{
                    width: childrenRect.width
                    height: 30
                }
                itemDelegate: Item {
                    Text {
                        // anchors.verticalCenter: parent.verticalCenter
                        color: styleData.selected || styleData.column == 0 ? "black" : styleData.textColor
                        elide: styleData.elideMode
                        text: styleData.column == 0 ? packageInfo.translate(styleData.value) : styleData.value
                        font.bold: styleData.column == 0
                        wrapMode: Text.WordWrap
                        horizontalAlignment: styleData.column == 0 ? Text.AlignHCenter : Text.AlignLeft
                    }
                }
          }
      }
      Tab {
          title: "包文件列表"
          ListView {
            id: fileListView
            anchors.fill: parent
            anchors.top: parent.top + 5
            spacing: 2
            model: appInfo.fileList
            // clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollBar.vertical: ScrollBar {}
            ScrollBar.horizontal: ScrollBar {}

            delegate: Rectangle {
                required property string modelData
                // width: fileListView.width - 10
                height: 30
                Text {
                    leftPadding: 5
                    rightPadding: 5
                    text: modelData
                }
            }
        }
      }

      style: TabViewStyle {
          frameOverlap: 1
          tabsAlignment: Qt.AlignHCenter
          tab: Rectangle {
              color: styleData.selected ? "steelblue" :"lightsteelblue"
              border.color:  "steelblue"
              implicitWidth: Math.max(text.width + 4, 80)
              implicitHeight: 20
              radius: 2
              Text {
                  id: text
                  anchors.centerIn: parent
                  text: styleData.title
                  color: styleData.selected ? "white" : "black"
              }
          }
          frame: Rectangle { color: "white" }
      }
  }
}