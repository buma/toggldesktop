#include "timerwidget.h"
#include "ui_timerwidget.h"

#include <QApplication>

#include "toggl_api.h"

TimerWidget::TimerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TimerWidget),
    timer(0),
    duration(0),
    timeEntryAutocompleteNeedsUpdate(false)
{
    ui->setupUi(this);

    connect(TogglApi::instance, SIGNAL(displayStoppedTimerState()),
            this, SLOT(displayStoppedTimerState()));
    connect(TogglApi::instance, SIGNAL(displayRunningTimerState(TimeEntryView*)),
            this, SLOT(displayRunningTimerState(TimeEntryView*)));

    connect(TogglApi::instance, SIGNAL(displayTimeEntryAutocomplete(QVector<AutocompleteView*>)),
            this, SLOT(displayTimeEntryAutocomplete(QVector<AutocompleteView*>)));

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(focusChanged(QWidget*,QWidget*)));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

TimerWidget::~TimerWidget()
{
    timer->stop();

    delete ui;
}

void TimerWidget::focusChanged(QWidget *old, QWidget *now)
{
    if (old == ui->description && timeEntryAutocompleteNeedsUpdate)
    {
        displayTimeEntryAutocomplete(timeEntryAutocompleteUpdate);
    }
}

void TimerWidget::displayRunningTimerState(
    TimeEntryView *te)
{
    ui->start->setText("Stop");

    ui->description->setEditText(te->Description);
    ui->description->setEnabled(false);

    ui->duration->setText(te->Duration);
    ui->duration->setEnabled(false);

    ui->project->setText(te->ProjectAndTaskLabel);

    duration = te->DurationInSeconds;

    disconnect(this, SLOT(start()));
    disconnect(this, SLOT(stop()));

    connect(this, SIGNAL(buttonClicked()), this, SLOT(stop()));

    timer->start(1000);
}

void TimerWidget::displayStoppedTimerState()
{
    ui->start->setText("Start");

    if (!ui->description->hasFocus())
    {
        ui->description->setEditText("");
    }
    ui->description->setEnabled(true);

    if (!ui->duration->hasFocus())
    {
        ui->duration->setText("");
    }
    ui->duration->setEnabled(true);

    ui->project->setText("");

    duration = 0;

    disconnect(this, SLOT(start()));
    disconnect(this, SLOT(stop()));

    connect(this, SIGNAL(buttonClicked()), this, SLOT(start()));

    timer->stop();
}

void TimerWidget::on_start_clicked()
{
    emit buttonClicked();
}

void TimerWidget::start()
{
    uint64_t task_id(0);
    uint64_t project_id(0);
    QVariant data = ui->description->currentData();
    if (data.canConvert<AutocompleteView *>())
    {
        AutocompleteView *view = data.value<AutocompleteView *>();
        task_id = view->TaskID;
        project_id = view->ProjectID;
    }
    TogglApi::instance->start(ui->description->currentText(),
                              ui->duration->text(),
                              task_id,
                              project_id);
}

void TimerWidget::stop()
{
    TogglApi::instance->stop();
}

void TimerWidget::displayTimeEntryAutocomplete(
    QVector<AutocompleteView *> list)
{
    timeEntryAutocompleteUpdate = list;
    timeEntryAutocompleteNeedsUpdate = true;
    if (ui->description->hasFocus())
    {
        return;
    }
    ui->description->clear();
    ui->description->addItem("");
    foreach(AutocompleteView *view, timeEntryAutocompleteUpdate)
    {
        ui->description->addItem(view->Text, QVariant::fromValue(view));
    }
    timeEntryAutocompleteNeedsUpdate = false;
}

void TimerWidget::timeout()
{
    if (duration >= 0)
    {
        return;
    }
    ui->duration->setText(TogglApi::formatDurationInSecondsHHMMSS(duration));
}

void TimerWidget::on_description_editTextChanged(const QString &arg1)
{

}

void TimerWidget::on_description_currentIndexChanged(int index)
{
    QVariant data = ui->description->currentData();
    if (data.canConvert<AutocompleteView *>())
    {
        AutocompleteView *view = data.value<AutocompleteView *>();
        ui->description->setEditText(view->Description);
        ui->project->setText(view->ProjectAndTaskLabel);
    }
}