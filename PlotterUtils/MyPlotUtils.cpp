// Copyright (c) 2014-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "MyPlotUtils.hpp"
#include "MyPlotStyler.hpp"
#include "MyPlotPicker.hpp"
#include <QList>
#include <valarray>
#include <qwt_legend_data.h>
#include <qwt_plot_canvas.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <qwt_text.h>
#include <QMouseEvent>

QColor getDefaultCurveColor(const size_t whichCurve)
{
    switch(whichCurve % 12)
    {
    case 0: return Qt::blue;
    case 1: return Qt::green;
    case 2: return Qt::red;
    case 3: return Qt::cyan;
    case 4: return Qt::magenta;
    case 5: return Qt::yellow;
    case 6: return Qt::darkBlue;
    case 7: return Qt::darkGreen;
    case 8: return Qt::darkRed;
    case 9: return Qt::darkCyan;
    case 10: return Qt::darkMagenta;
    case 11: return Qt::darkYellow;
    };
    return QColor();
}

QColor pastelize(const QColor &c)
{
    //Pastels have high value and low to intermediate saturation:
    //http://en.wikipedia.org/wiki/Pastel_%28color%29
    return QColor::fromHsv(c.hue(), int(c.saturationF()*128), int(c.valueF()*64)+191);
}

/***********************************************************************
 * Custom QwtPlotCanvas that accepts the mousePressEvent
 * (needed to make mouse-based events work within QGraphicsItem)
 **********************************************************************/
class MyQwtPlotCanvas : public QwtPlotCanvas
{
    Q_OBJECT
public:
    MyQwtPlotCanvas(QwtPlot *parent):
        QwtPlotCanvas(parent)
    {
        return;
    }
protected:
    void mousePressEvent(QMouseEvent *event)
    {
        QwtPlotCanvas::mousePressEvent(event);
        event->accept();
    }
};

/***********************************************************************
 * Custom QwtPlot implementation
 **********************************************************************/
MyQwtPlot::MyQwtPlot(QWidget *parent):
    QwtPlot(parent),
    _zoomer(nullptr)
{
    this->setCanvas(new MyQwtPlotCanvas(this));
    _zoomer = new MyPlotPicker(this->canvas());
    qRegisterMetaType<QList<QwtLegendData>>("QList<QwtLegendData>"); //missing from qwt
    qRegisterMetaType<std::valarray<float>>("std::valarray<float>"); //used for plot data
    connect(this, SIGNAL(itemAttached(QwtPlotItem *, bool)), this, SLOT(handleItemAttached(QwtPlotItem *, bool)));
}

void MyQwtPlot::setTitle(const QString &text)
{
    QwtPlot::setTitle(MyPlotTitle(text));
}

void MyQwtPlot::setAxisTitle(const int id, const QString &text)
{
    QwtPlot::setAxisTitle(id, MyPlotAxisTitle(text));
}

void MyQwtPlot::updateChecked(QwtPlotItem *item)
{
    auto legend = dynamic_cast<QwtLegend *>(this->legend());
    if (legend == nullptr) return; //no legend
    auto info = legend->legendWidget(this->itemToInfo(item));
    auto label = dynamic_cast<QwtLegendLabel *>(info);
    if (label == nullptr) return; //no label
    label->setChecked(item->isVisible());
    this->updateLegend();
}

void MyQwtPlot::handleItemAttached(QwtPlotItem *, bool on)
{
    //apply stashed visibility when the item count matches
    //this handles curves which are added on-demand
    const auto items = this->itemList();
    if (on and items.size() == _visible.size())
    {
        for (int i = 0; i < items.size(); i++)
        {
            items[i]->setVisible(_visible.at(i));
            this->updateChecked(items[i]);
        }
        _visible.clear();
    }
}

QVariant MyQwtPlot::state(void) const
{
    QVariantMap state;

    //zoom stack
    QVariantList stack;
    for (const auto &rect : zoomer()->zoomStack()) stack.append(rect);
    state["stack"] = stack;
    state["index"] = zoomer()->zoomRectIndex();

    //item visibility
    const auto items = this->itemList();
    QBitArray visible(items.size());
    for (int i = 0; i < items.size(); i++)
    {
        visible.setBit(i, items[i]->isVisible());
    }
    state["visible"] = visible;

    return state;
}

void MyQwtPlot::setState(const QVariant &state)
{
    const auto map = state.toMap();

    //zoom stack
    QStack<QRectF> stack;
    for (const auto &rect : map["stack"].toList()) stack.push(rect.toRectF());
    zoomer()->setZoomStack(stack, map["index"].toInt());

    //item visibility
    const auto items = this->itemList();
    _visible = map["visible"].toBitArray();
    for (int i = 0; i < items.size() and i < _visible.size(); i++)
    {
        items[i]->setVisible(_visible.at(i));
        this->updateChecked(items[i]);
    }
}

#include "MyPlotUtils.moc"
