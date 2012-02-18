/*
    IanniX — a graphical real-time open-source sequencer for digital art
    Copyright (C) 2010-2012 — IanniX Association

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nxcursor.h"

NxCursor::NxCursor(NxObjectFactoryInterface *parent, QTreeWidgetItem *ccParentItem, UiRenderOptions *_renderOptions) :
    NxObject(parent, ccParentItem, _renderOptions) {
    curve = 0;
    setTimeLocal(0);
    nextTimeOld = 0;
    timeOld = 0;
    time = 0;
    previousCursorReliable = false;
    cursorPoly = NxPolygon(4);
    cursorPoly[0] = NxPoint();
    cursorPoly[1] = NxPoint();
    cursorPoly[2] = NxPoint();
    cursorPoly[3] = NxPoint();
    setNbLoop(0);
    setStart("1");
    setTimeFactor(1);
    setTimeFactorF(1);
    setWidth(1);
    setDepth(0);
    setTime(0);
    setSize(2);
    setLineFactor(1);
    setLineStipple(0xFFFF);
    setColorActive("cursor_active");
    setColorInactive("cursor_inactive");
    setTimeStartOffset(0);
    setTimeEndOffset(-1);
    setTimeInitialOffset(0);
    setEasing(0);
    setBoundsSource("-10 10 10 -10");
    setBoundsTarget("0 1 1 0");
    boundsSourceIsBoundingRect = true;
}

void NxCursor::setTime(qreal delta) {
    if(curve) {
        //TODO overflow si speed négatif + negative values
        //timeLocalAbsolute += delta * timeFactor * timeFactorF * qAbs(start.at(nbLoop % start.count()));
        factors = timeFactor * timeFactorF * start.at(nbLoop % start.count());
        timeLocalAbsolute += delta * qAbs(factors);
        delta *= factors;
        timeLocalOld = timeLocal;
        if(time >= 0)
            timeLocal += delta;
        else
            timeLocal = 0;

        qreal timeInitialOffsetReal = timeInitialOffset * qAbs(factors);
        qreal timeStartOffsetReal   = timeStartOffset   * qAbs(factors);
        qreal timeEndOffsetReal     = timeEndOffset     * qAbs(factors);
        qreal timeLocalAbsoluteCopy = timeLocalAbsolute + timeInitialOffsetReal;
        qreal fakeCurveLength = curve->getPathLength() - timeStartOffsetReal;
        if(timeEndOffset > 0)
            fakeCurveLength = timeEndOffsetReal - timeStartOffsetReal;
        nbLoop = 0;
        bool patternSignOld = true;
        while((timeLocalAbsoluteCopy > fakeCurveLength) && (fakeCurveLength > 0)) {
            patternSignOld = (start.at(nbLoop % start.count()) > 0);
            nbLoop++;
            timeLocalAbsoluteCopy -= fakeCurveLength;
        }

        //Preparation of time difference
        if(!previousCursorReliable)
            timeOld = nextTimeOld;
        else
            timeOld = time;
        nextTimeOld = timeOld;
        previousCursorReliable = true;

        //Time calculation
        bool patternSign = (factors >= 0);// && (timeFactorF >= 0) && (timeFactor >= 0);
        if(patternSign)
            time = timeLocalAbsoluteCopy / fakeCurveLength;//timeLocalAbsoluteCopy / curve->getPathLength();
        else
            time = (fakeCurveLength - timeLocalAbsoluteCopy) / fakeCurveLength;//(fakeCurveLength - timeLocalAbsoluteCopy) / curve->getPathLength();

        if(time <= 0)
            previousCursorReliable = false;

        //Loop
        if(nbLoop != nbLoopOld) {
            previousCursorReliable = false;
            nextTimeOld = qRound(time)    / curve->getPathLength() * fakeCurveLength + timeStartOffsetReal / curve->getPathLength();
            time        = qRound(timeOld) / curve->getPathLength() * fakeCurveLength + timeStartOffsetReal / curve->getPathLength();
            //qDebug("----------------------------------------------------");
        }
        else
            time = time / curve->getPathLength() * fakeCurveLength + timeStartOffsetReal / curve->getPathLength();
        //qDebug("%d -> %d \t %d -> %d \t %f -> %f (%f)\t%d", nbLoopOld, nbLoop, patternSignOld, patternSign, timeOld, time, nextTimeOld, previousCursorReliable);

        //Finaly
        nbLoopOld = nbLoop;
        calculate();

        //Easing
        /*
        qreal timeLocalEasing = timeLocal;
        if((0 < timeLocalAbsolute) && (timeLocalAbsolute < easingStartDuration)) {
            timeLocalEasing = easingStartDuration * easing.valueForProgress(timeLocalAbsolute/easingStartDuration);
            if(timeLocal < 0)
                timeLocalEasing = -timeLocalEasing;
        }
        */
    }
}


void NxCursor::calculate() {
    //Cursor line
    if((curve) && (curve->getPathLength() > 0)) {
        //qreal timeReal = ((qExp(5*time) - 1) / (qExp(5*1) - 1));
        qreal timeReal = easing.valueForProgress(time), timeOldReal = easing.valueForProgress(timeOld);

        cursorPos = curve->getPointAt(timeReal) + curve->getPos();
        if(timeReal == 0)
            cursorAngle = -curve->getAngleAt(timeReal + 0.001);
        else if(timeReal == 1)
            cursorAngle = -curve->getAngleAt(timeReal - 0.001);
        else
            cursorAngle = -curve->getAngleAt(timeReal);

        cursorPosOld = curve->getPointAt(timeOldReal) + curve->getPos();
        if(timeOldReal == 0)
            cursorAngleOld = -curve->getAngleAt(timeOldReal + 0.001);
        else if(timeOldReal == 1)
            cursorAngleOld = -curve->getAngleAt(timeOldReal - 0.001);
        else
            cursorAngleOld = -curve->getAngleAt(timeOldReal);
    }
    else {
        NxPoint cursorPosDelta = pos - cursorPos;
        cursorPos = pos;
        previousCursorReliable = true;

        if((cursorPosDelta.x() > 0) && (cursorPosDelta.y() >= 0))
            cursorAngle = (qAtan(cursorPosDelta.y() / cursorPosDelta.x())) * 180.0F / M_PI;
        else if((cursorPosDelta.x() <= 0) && (cursorPosDelta.y() > 0))
            cursorAngle = (-qAtan(cursorPosDelta.x() / cursorPosDelta.y()) + M_PI_2) * 180.0F / M_PI;
        else if((cursorPosDelta.x() < 0) && (cursorPosDelta.y() <= 0))
            cursorAngle = (qAtan(cursorPosDelta.y() / cursorPosDelta.x()) + M_PI) * 180.0F / M_PI;
        else if((cursorPosDelta.x() >= 0) && (cursorPosDelta.y() < 0))
            cursorAngle = (-qAtan(cursorPosDelta.x() / cursorPosDelta.y()) + 3 * M_PI_2) * 180.0F / M_PI;
    }
    if(cursorAngle != cursorAngle)
        cursorAngle = 0;
    cursorRelativePos = getCursorValue(cursorPos);

    //Calculate polygon (from previous cursor to actual cursor)
    QLineF cursorTmp = QTransform().rotate(cursorAngle + 90).map(QLineF(-width/2, 0, width/2, 0));
    cursor = NxLine(cursorTmp.x1(), cursorTmp.y1(), cursorTmp.x2(), cursorTmp.y2());
    cursor.translate(cursorPos);
    QLineF cursorOldTmp = QTransform().rotate(cursorAngleOld + 90).map(QLineF(-width/2, 0, width/2, 0));
    cursorOld = NxLine(cursorOldTmp.x1(), cursorOldTmp.y1(), cursorOldTmp.x2(), cursorOldTmp.y2());
    cursorOld.translate(cursorPosOld);

    QLineF diag1(cursor.p1().x(),cursor.p1().y(),cursor.p2().x(),cursor.p2().y());
    QLineF diag2(cursorOld.p1().x(),cursorOld.p1().y(),cursorOld.p2().x(),cursorOld.p2().y());
    QPointF intersectionPoint;

    if (diag1.intersect(diag2, &intersectionPoint) == QLineF::BoundedIntersection) {  ////GC//// Fix crossed diagonal in cursorPoly
        cursorPoly[0] = cursor.p1();
        cursorPoly[1] = cursorOld.p1();
        cursorPoly[2] = cursor.p2();
        cursorPoly[3] = cursorOld.p2();
    } else {
        cursorPoly[0] = cursorOld.p1();
        cursorPoly[1] = cursor.p1();
        cursorPoly[2] = cursor.p2();
        cursorPoly[3] = cursorOld.p2();
    }

    calcBoundingRect();

    previousCursor = cursor;
}


void NxCursor::paint() {
    //Color
    if(active)
        color = (colorActive != "")?(renderOptions->colors.value(colorActive)):(colorActiveColor);
    else
        color = (colorInactive != "")?(renderOptions->colors.value(colorInactive)):(colorInactiveColor);

    if(color.alpha() > 0) {
        //Size of cursors
        qreal cacheSize = renderOptions->objectSize;

        //Mouse hover
        if(selectedHover)
            color = renderOptions->colors.value("object_hover");
        if(selected)
            color = renderOptions->colors.value("object_selection");

        //Start
        if((renderOptions->paintCursors) && (renderOptions->paintThisGroup) && ((renderOptions->paintZStart <= pos.z()) && (pos.z() <= renderOptions->paintZEnd)))
            glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
        else
            glColor4f(color.redF(), color.greenF(), color.blueF(), 0.1);


        //Label
        if((renderOptions->paintLabel) && (label != ""))
            renderOptions->render->renderText(cursorPos.x(), cursorPos.y(), cursorPos.z(), label, renderOptions->renderFont);
        if(selectedHover) {
            qreal startY = 0;
            foreach(const QString & messageLabelItem, messageLabel) {
                renderOptions->render->renderText(cursorPos.x(), cursorPos.y() + startY, cursorPos.z(), messageLabelItem, renderOptions->renderFont);
                startY -= cacheSize * 1.5;
            }
        }

        //Cursor chasse-neige
        if((time >= 0) && (start.at(nbLoop % start.count()) != 0)) {
            if(previousCursorReliable) {
                glLineWidth(size/4);
                glEnable(GL_LINE_STIPPLE);
                glLineStipple(lineFactor, lineStipple);
                glBegin(GL_LINE_LOOP);
                glVertex3f(cursorPoly.at(0).x(), cursorPoly.at(0).y(), cursorPoly.at(0).z());
                glVertex3f(cursorPoly.at(1).x(), cursorPoly.at(1).y(), cursorPoly.at(1).z());
                glVertex3f(cursorPoly.at(2).x(), cursorPoly.at(2).y(), cursorPoly.at(2).z());
                glVertex3f(cursorPoly.at(3).x(), cursorPoly.at(3).y(), cursorPoly.at(3).z());
                glEnd();
                glDisable(GL_LINE_STIPPLE);
            }

            glLineWidth(size);
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(lineFactor, lineStipple);
            glBegin(GL_LINE_STRIP);
            glVertex3f(cursorPoly.at(1).x(), cursorPoly.at(1).y(), cursorPoly.at(1).z());
            glVertex3f(cursorPoly.at(2).x(), cursorPoly.at(2).y(), cursorPoly.at(2).z());
            glEnd();
            glDisable(GL_LINE_STIPPLE);

            //Cursor reader
            if(((timeLocal - timeLocalOld) != 0) || (!curve)) {
                glLineWidth(1);
                glPushMatrix();
                glTranslatef(cursorPos.x(), cursorPos.y(), cursorPos.z());
                glRotatef(cursorAngle, 0, 0, 1);
                qreal size2 = cacheSize/2;
                glBegin(GL_TRIANGLE_FAN);
                if((timeLocal - timeLocalOld) > 0)  glVertex3f(size2, 0, 0);
                else                                glVertex3f(-size2, 0, 0);
                glVertex3f(0, -size2, 0);
                glVertex3f(0, 0, size2/2);
                glVertex3f(0, size2, 0);
                glVertex3f(0, 0, -size2/2);
                glVertex3f(0, -size2, 0);
                glEnd();

                glPopMatrix();
            }
        }
    }
}

void NxCursor::trig() {
    if((((timeLocal - timeLocalOld) != 0) || (!curve)) && (canSendOsc())) {
        factory->sendMessage(this, 0, this);
        incMessageId();
    }
}

bool NxCursor::contains(NxTrigger *trigger) {
    if((trigger->getActive()) && (trigger->getPos().z() == cursorPoly[0].z()) && (cursorPoly.containsPoint(trigger->getPos(), Qt::OddEvenFill)))
        return true;
    else
        return false;
}
bool NxCursor::trig(NxCurve *collisionCurve) {
    if((performCollision) && (collisionCurve) && (collisionCurve->getActive()) && (collisionCurve != curve)) {
        NxPoint collisionPoint;
        qreal percent = collisionCurve->intersects(boundingRect, &collisionPoint);
        if(percent >= 0) {
            factory->sendMessage(this, 0, this, collisionCurve, collisionPoint, getCursorValue(collisionPoint));
            return true;
        }
        return false;
    }
    else
        return false;
}
