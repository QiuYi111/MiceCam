import QtQuick
import "../theme"

Canvas {
    id: root
    property string name: ""
    property color color: Theme.textPrimary
    property real size: 18
    
    width: size
    height: size
    
    onColorChanged: requestPaint()
    onNameChanged: requestPaint()
    
    onPaint: {
        var ctx = getContext("2d");
        ctx.reset();
        ctx.strokeStyle = color;
        ctx.fillStyle = color;
        ctx.lineWidth = 1.5;
        ctx.lineCap = "round";
        ctx.lineJoin = "round";
        
        var w = width;
        var h = height;
        
        if (name === "camera") {
            ctx.beginPath();
            ctx.rect(1, 4, 11, 10);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(12, 7); ctx.lineTo(16, 5); ctx.lineTo(16, 13); ctx.lineTo(12, 11);
            ctx.closePath(); ctx.fill();
        } else if (name === "encoding") {
            for (var i = 0; i < 3; i++) {
                var y = 4 + i * 5;
                ctx.beginPath(); ctx.moveTo(2, y); ctx.lineTo(16, y); ctx.stroke();
                ctx.beginPath(); ctx.arc(4 + i * 4, y, 2, 0, Math.PI * 2); ctx.fill();
            }
        } else if (name === "alerts") {
            ctx.beginPath();
            ctx.moveTo(9, 2); ctx.lineTo(9, 3);
            ctx.moveTo(4, 13); ctx.lineTo(14, 13);
            ctx.arc(9, 13, 5, Math.PI, 0);
            ctx.lineTo(14, 13);
            ctx.stroke();
            ctx.beginPath(); ctx.arc(9, 15, 2, 0, Math.PI); ctx.stroke();
        } else if (name === "logging") {
            ctx.beginPath();
            ctx.rect(3, 2, 12, 14);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(6, 6); ctx.lineTo(12, 6);
            ctx.moveTo(6, 9); ctx.lineTo(12, 9);
            ctx.moveTo(6, 12); ctx.lineTo(10, 12);
            ctx.stroke();
        } else if (name === "about") {
            ctx.beginPath(); ctx.arc(9, 9, 7, 0, Math.PI * 2); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(9, 7); ctx.lineTo(9, 13); ctx.stroke();
            ctx.beginPath(); ctx.arc(9, 5, 0.5, 0, Math.PI * 2); ctx.fill();
        } else if (name === "gear") {
            ctx.beginPath(); ctx.arc(9, 9, 3, 0, Math.PI * 2); ctx.stroke();
            for (var i = 0; i < 8; i++) {
                ctx.save(); ctx.translate(9, 9); ctx.rotate(i * Math.PI / 4);
                ctx.beginPath(); ctx.moveTo(0, -5); ctx.lineTo(0, -7); ctx.stroke();
                ctx.restore();
            }
        } else if (name === "fullscreen") {
            ctx.beginPath();
            ctx.moveTo(3, 7); ctx.lineTo(3, 3); ctx.lineTo(7, 3);
            ctx.moveTo(11, 3); ctx.lineTo(15, 3); ctx.lineTo(15, 7);
            ctx.moveTo(15, 11); ctx.lineTo(15, 15); ctx.lineTo(11, 15);
            ctx.moveTo(7, 15); ctx.lineTo(3, 15); ctx.lineTo(3, 11);
            ctx.stroke();
        } else if (name === "clock") {
            ctx.beginPath(); ctx.arc(9, 9, 7, 0, Math.PI * 2); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(9, 9); ctx.lineTo(9, 5); ctx.moveTo(9, 9); ctx.lineTo(12, 9); ctx.stroke();
        } else if (name === "film") {
            ctx.beginPath(); ctx.rect(2, 4, 14, 10); ctx.stroke();
            for (var i = 0; i < 4; i++) {
                ctx.fillRect(4 + i * 3, 5, 2, 2);
                ctx.fillRect(4 + i * 3, 11, 2, 2);
            }
        } else if (name === "chart") {
            ctx.beginPath();
            ctx.moveTo(2, 14); ctx.lineTo(6, 8); ctx.lineTo(10, 11); ctx.lineTo(16, 4);
            ctx.stroke();
        } else if (name === "disk") {
            ctx.beginPath();
            ctx.rect(3, 4, 12, 10);
            ctx.stroke();
            ctx.beginPath(); ctx.moveTo(3, 9); ctx.lineTo(15, 9); ctx.stroke();
            ctx.fillRect(11, 6, 2, 2);
        } else if (name === "check") {
            ctx.beginPath();
            ctx.moveTo(4, 9); ctx.lineTo(8, 13); ctx.lineTo(14, 5);
            ctx.stroke();
        } else if (name === "chevron-right") {
            ctx.beginPath(); ctx.moveTo(6, 4); ctx.lineTo(11, 9); ctx.lineTo(6, 14); ctx.stroke();
        } else if (name === "eye") {
            ctx.beginPath(); ctx.arc(9, 9, 3, 0, Math.PI * 2); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(2, 9); ctx.bezierCurveTo(2, 9, 9, 2, 16, 9);
            ctx.bezierCurveTo(16, 9, 9, 16, 2, 9); ctx.stroke();
        } else if (name === "close") {
            ctx.beginPath();
            ctx.moveTo(4, 4); ctx.lineTo(14, 14);
            ctx.moveTo(14, 4); ctx.lineTo(4, 14);
            ctx.stroke();
        } else if (name === "warning") {
            ctx.beginPath();
            ctx.moveTo(9, 2); ctx.lineTo(16, 15); ctx.lineTo(2, 15); ctx.closePath();
            ctx.stroke();
            ctx.beginPath(); ctx.moveTo(9, 7); ctx.lineTo(9, 10); ctx.stroke();
            ctx.beginPath(); ctx.arc(9, 12.5, 0.7, 0, Math.PI * 2); ctx.fill();
        }
    }
}
