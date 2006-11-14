// Copyright (c) 2003  INRIA Sophia-Antipolis (France) and
//                     Max-Planck-Institute Saarbruecken (Germany).
// All rights reserved.
//
// Authors : Monique Teillaud <Monique.Teillaud@sophia.inria.fr>
//           Sylvain Pion     <Sylvain.Pion@sophia.inria.fr>
//           Radu Ursu
// 
// Partially supported by the IST Programme of the EU as a Shared-cost
// RTD (FET Open) Project under Contract No  IST-2000-26473 
// (CGAL - Effective Computational Geometry for Curves and Surfaces) 


// TODO :
// - Add the possibility to input full circles.
// - locate the cell containing a point.
// - Move more basic IO routines in include/CGAL/IO/
// - File menu

#include <CGAL/basic.h>

// if QT is not installed, a message will be issued at runtime.
#ifndef CGAL_USE_QT
#include <iostream>
int main() {
  std::cout << "Sorry, this demo needs QT..." << std::endl;
  return 0;
}
#else

#include <fstream>

#include <CGAL/Cartesian.h>
#include <CGAL/point_generators_2.h>

#include <CGAL/MP_Float.h>
#include <CGAL/Gmpq.h>

#include <CGAL/Algebraic_kernel_for_circles_2_2.h>

#include <CGAL/IO/Qt_widget.h>
#include <CGAL/IO/Qt_widget_standard_toolbar.h>
#include <CGAL/IO/Qt_help_window.h>
#include <CGAL/IO/Qt_widget_layer.h>

#include <qplatinumstyle.h>
#include <qapplication.h>
#include <qmainwindow.h>
#include <qstatusbar.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qtoolbutton.h>
#include <qtoolbar.h>
#include <qfiledialog.h>
#include <qtimer.h>

#include <CGAL/IO/Qt_widget_circular_arc_2.h>
#include <CGAL/IO/Qt_widget_circular_arc_endpoint_2.h>


#include "Qt_widget_get_segment.h"
#include "Qt_widget_get_arc.h"
#include "trash.xpm"
#include "get_arc.xpm"

#include <CGAL/Circular_kernel_2.h>
#include <CGAL/Arr_circular_line_arc_traits.h>
#include <CGAL/Arr_circular_arc_traits_tracer.h>
#include <CGAL/IO/Dxf_variant_reader.h>

typedef CGAL::Quotient<CGAL::MP_Float>                      NT;  
typedef CGAL::Cartesian<NT>                                 Linear_k;
typedef CGAL::Algebraic_kernel_for_circles_2_2<NT>          Algebraic_k;
typedef CGAL::Circular_kernel_2<Linear_k,Algebraic_k>       Circular_k;

typedef Circular_k::Line_arc_2                              Line_arc_2;
typedef Circular_k::Segment_2                               Segment;
typedef Circular_k::Circular_arc_2                          Circular_arc_2;
typedef boost::variant< Circular_arc_2, Line_arc_2 >        Arc_2;
typedef std::vector<Arc_2>                                    ArcContainer;

#ifndef CGAL_CURVED_KERNEL_DEBUG
typedef CGAL::Arr_circular_line_arc_traits
<Circular_k, Circular_arc_2, Line_arc_2>                    Traits;
#else
typedef CGAL::Arr_circular_line_arc_traits
<Circular_k, Circular_arc_2, Line_arc_2>                    Traits0;
typedef CGAL::Circular_arc_traits_tracer<Traits0>           Traits;
#endif

typedef Traits::Point_2                             Point_2;
typedef Traits::Curve_2                             Conic_arc_2;

typedef Traits::X_monotone_curve_2                  X_monotone_curve_2;



const QString my_title_string("CGAL :: "
                              "Planar Map of Intersecting Circular Arcs");

// This layer controls the drawing of the arc_container.
struct Qt_layer_show_ch
  : public CGAL::Qt_widget_layer
{
  ArcContainer _arc_container;
  bool show_arcs;

public:

  Qt_layer_show_ch()
    : show_arcs(true) {}

  const ArcContainer &
  arc_container() const { return _arc_container; }
  ArcContainer &
  arc_container() { return _arc_container; }

  void swap_show() { show_arcs = ! show_arcs; }

  void draw()
  {
    if (!show_arcs)
      return;

    *widget << CGAL::BLUE;
    for (ArcContainer::const_iterator cit = arc_container().begin();
         cit != arc_container().end(); ++cit){
      if(const Line_arc_2* line = boost::get<Line_arc_2>( &(*cit))){
	*widget << Segment(Circular_k::Point_2(to_double(line->source().x()),
					     to_double(line->source().y())),
			   Circular_k::Point_2(to_double(line->target().x()),
					     to_double(line->target().y())));
      }
      else if (const Circular_arc_2* arc = boost::get<Circular_arc_2>( &(*cit))){
	*widget << *arc;
      }
    }
  }
};


class MyWindow
  : public QMainWindow
{
  Q_OBJECT

public:

  MyWindow(int w, int h)
    : something_changed(true)
  {
    widget = new CGAL::Qt_widget(this);
    setCentralWidget(widget);

    // create a timer for checking if something changed
    QTimer *timer = new QTimer( this );
    connect( timer, SIGNAL(timeout()), this, SLOT(timer_done()) );
    timer->start( 200, FALSE );

    // file menu
    QPopupMenu * file = new QPopupMenu( this );
    menuBar()->insertItem( "&File", file );
    file->insertItem("&New", this, SLOT(new_instance()), CTRL+Key_N);
    file->insertItem("New &Window", this, SLOT(new_window()), CTRL+Key_W);
    file->insertSeparator();
    file->insertItem("&Load Arcs", this, SLOT(load_arcs()), CTRL+Key_L);
    file->insertItem("&Save Arcs", this, SLOT(save_arcs()), CTRL+Key_S);
    file->insertSeparator();
    file->insertItem("Print", widget, SLOT(print_to_ps()), CTRL+Key_P);
    file->insertSeparator();
    file->insertItem( "&Close", this, SLOT(close()), CTRL+Key_X );
    file->insertItem( "&Quit", qApp, SLOT( closeAllWindows() ), CTRL+Key_Q );


    // help menu
    QPopupMenu * help = new QPopupMenu( this );
    menuBar()->insertItem( "&Help", help );
    help->insertItem("How To", this, SLOT(howto()), Key_F1);
    help->insertSeparator();
    help->insertItem("&About", this, SLOT(about()), CTRL+Key_A );
    help->insertItem("About &Qt", this, SLOT(aboutQt()) );

    // the standard toolbar
    stoolbar = new CGAL::Qt_widget_standard_toolbar (widget, this, "ST");

    // my tool bar
    QToolBar * layers_toolbar = new QToolBar("Tools", this,
                                             QMainWindow::Top, TRUE, "Tools");


    // the button controlling if we show the input arcs
    QToolButton * show_container_button =
                   new QToolButton(QPixmap((const char**)::get_arc),
                                   "Show Original arcs",
                                   0,
                                   this,
                                   SLOT(show_original_arcs()),
                                   layers_toolbar,
                                   "Show Original arcs");
    show_container_button->setToggleButton(true);

    show_container_button->toggle();
    connect(show_container_button, SIGNAL(stateChanged(int)),
            &testlayer, SLOT(stateChanged(int)));

    // this button clears the content of the arc container and the PMWX.
    QToolButton * clear_button =
                       new QToolButton(QPixmap((const char**)trash),
                                       "Clear",
                                       0,
                                       this,
                                       SLOT(clear_container()),
                                       layers_toolbar,
                                       "Clear");

    connect(clear_button, SIGNAL(stateChanged(int)),
            &testlayer, SLOT(stateChanged(int)));


    *widget << CGAL::LineWidth(2) << CGAL::BackgroundColor(CGAL::WHITE);

    resize(w,h);
    widget->set_window(-1, 1, -1, 1);
    widget->setMouseTracking(TRUE);

    // layers
    widget->attach(&testlayer);
    get_arc_layer = new CGAL::Qt_widget_get_arc<Circular_k>;
    get_segment_layer =  new CGAL::Qt_widget_get_segment<Circular_k>;
   // connect(get_arc_layer, SIGNAL(new_object_time()), this, SLOT(get_arc()));
   // connect(get_segment_layer, SIGNAL(new_object_time()), this, SLOT(get_arc()));
  }

public slots:

  void new_instance()
  {
    widget->lock();
    widget->clear_history();
    widget->set_window(-1.1, 1.1, -1.1, 1.1);
		// set the Visible Area to the Interval
    widget->unlock();
    something_changed = true;
  }


  void show_original_arcs()
  {
    testlayer.swap_show();
    something_changed = true;
    widget->redraw();
  }

  void clear_container()
  {
    arc_container().clear();
    something_changed = true;
    widget->redraw();
  }

private slots:

  void about()
  {
    QMessageBox::about(this, my_title_string,
	"This is a demo of CGAL's Planar Map of intersecting Circle Arcs\n"
        "Using the Circular_kernel by Sylvain Pion and Monique Teillaud");
  }

  void aboutQt()
  {
    QMessageBox::aboutQt(this, my_title_string);
  }

  void howto()
  {
    CGAL::Qt_help_window *help =
          new CGAL::Qt_help_window("help/index.html", ".", 0, "help viewer");
    help->resize(400, 400);
    help->setCaption("Demo HowTo");
    help->show();
  }

  void new_window()
  {
    MyWindow *ed = new MyWindow(500, 500);
    ed->setCaption("Layer");
    ed->widget->clear_history();
    ed->widget->set_window(-1.1, 1.1, -1.1, 1.1);
    ed->show();
    something_changed = true;
  }

  void timer_done()
  {
    if (something_changed)
      widget->redraw();
    something_changed = false;
  }

  void load_arcs()
  {
    QString s( QFileDialog::getOpenFileName( QString::null,
                            "DXF files (*.dxf)", this ) );
    if ( s.isEmpty() )
        return;

    //std::ifstream in(s);
    //CGAL::set_ascii_mode(in);
    
    //std::istream_iterator<Arc_2> begin(in), end;
    //ArcContainer arcs(begin, end);
    //arc_container().swap(arcs);

    //to read dxf files
    std::ifstream in(s);
    CGAL::set_ascii_mode(in);
    ArcContainer arcs;
    CGAL::variant_load<Circular_k, Circular_arc_2, Line_arc_2>(in, std::back_inserter(arcs));
    arc_container().swap(arcs);


    //update_pmwx();
    stoolbar->clear_history();
    something_changed = true;
  }

  void save_arcs()
  {
    QString fileName =
      QFileDialog::getSaveFileName( "arcs.cgal",
                                  "CGAL files (*.cgal)", this );
    if ( !fileName.isNull() ) {
      // got a file name
      std::ofstream out(fileName);
      CGAL::set_ascii_mode(out);
     // std::copy(arc_container().begin(), arc_container().end(),
     //           std::ostream_iterator<Arc_2>(out, "\n"));
    }
  }

private:

  const ArcContainer & arc_container() const
  { return testlayer.arc_container(); }
  ArcContainer & arc_container()
  { return testlayer.arc_container(); }

  CGAL::Qt_widget                      *widget;
  CGAL::Qt_widget_standard_toolbar     *stoolbar;
  bool                                 something_changed;
  Qt_layer_show_ch                     testlayer;

  CGAL::Qt_widget_get_segment<Circular_k>  *get_segment_layer;
  CGAL::Qt_widget_get_arc<Circular_k>     *get_arc_layer;
};

#include "demo_viewer.moc"

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);
  MyWindow widget(800, 700); // physical window size
  app.setMainWidget(&widget);
  widget.setCaption(my_title_string);
  widget.setMouseTracking(TRUE);
  widget.show();
  return app.exec();
}

#endif // CGAL_USE_QT
