HEADERS = \
      ./ctrl/ctrledit.h       \
      ./ctrl/ctrlcanvas.h           \
      ./ctrl/ctrlpanel.h            \
      ./debug.h         \
      ./midiport.h            \
      ./app.h           \
      ./eventbase.h           \
      ./key.h           \
      ./liste/listedit.h            \
      ./liste/editevent.h           \
      ./mixer/mstrip.h        \
      ./mixer/auxknob.h       \
      ./mixer/meter.h         \
      ./mixer/panknob.h       \
      ./mixer/strip.h         \
      ./mixer/rack.h          \
      ./mixer/routedialog.h         \
      ./mixer/amixer.h        \
      ./mixer/astrip.h        \
      ./mtc.h           \
      ./pos.h           \
      ./sig.h           \
      ./xml.h           \
      ./miditransform.h       \
      ./route.h         \
      ./globaldefs.h          \
      ./appearance.h          \
      ./midievent.h           \
      ./midieditor.h          \
      ./confmport.h           \
      ./patchbay/patchbay.h         \
      ./tempo.h         \
      ./audioprefetch.h       \
      ./event.h         \
      ./waveevent.h           \
      ./midiedit/pianoroll.h        \
      ./midiedit/cmd.h        \
      ./midiedit/dlist.h            \
      ./midiedit/prcanvas.h         \
      ./midiedit/quantconfig.h            \
      ./midiedit/ecanvas.h          \
      ./midiedit/drumedit.h         \
      ./midiedit/drummap.h          \
      ./midiedit/piano.h            \
      ./midiedit/dcanvas.h          \
      ./conf.h          \
      ./ctrl.h          \
      ./driver/jackaudio.h          \
      ./driver/alsamidi.h           \
      ./driver/audiodev.h           \
      ./fastlog.h       \
      ./mpevent.h       \
      ./synth.h         \
      ./cobject.h       \
      ./track.h         \
      ./evdata.h        \
      ./marker/markerview.h         \
      ./marker/marker.h       \
      ./master/tscale.h       \
      ./master/lmaster.h            \
      ./master/masteredit.h         \
      ./master/master.h       \
      ./gconfig.h       \
      ./midi.h          \
      ./waveedit/waveedit.h         \
      ./waveedit/waveview.h         \
      ./mplugins/midifilterimpl.h         \
      ./mplugins/mittranspose.h           \
      ./mplugins/random.h           \
      ./mplugins/mrconfig.h         \
      ./mplugins/midiitransform.h         \
      ./mplugins/rhythm.h           \
      ./mplugins/mitplugin.h        \
      ./node.h          \
      ./part.h          \
      ./value.h         \
      ./song.h          \
      ./sync.h          \
      ./undo.h          \
      ./ladspa.h        \
      ./wave.h          \
      ./device.h        \
      ./instruments/midictrledit.h        \
      ./instruments/minstrument.h         \
      ./instruments/editinstrument.h            \
      ./icons.h         \
      ./thread.h        \
      ./arranger/tlist.h            \
      ./arranger/arranger.h         \
      ./arranger/alayout.h          \
      ./arranger/pcanvas.h          \
      ./globals.h       \
      ./audio.h         \
      ./transpose.h           \
      ./transport.h           \
      ./widgets/section.h           \
      ./widgets/mixdowndialog.h           \
      ./widgets/dimap.h       \
      ./widgets/fontsel.h           \
      ./widgets/lcombo.h            \
      ./widgets/gcombo.h           \
      ./widgets/pitchedit.h         \
      ./widgets/tb1.h         \
      ./widgets/gatetime.h          \
      ./widgets/comboQuant.h        \
      ./widgets/ctrlcombo.h         \
      ./widgets/comment.h           \
      ./widgets/filedialog.h        \
      ./widgets/sclif.h       \
      ./widgets/scrollscale.h       \
      ./widgets/bigtime.h           \
      ./widgets/metronome.h         \
      ./widgets/combobox.h          \
      ./widgets/ttoolbutton.h       \
      ./widgets/drange.h            \
      ./widgets/scldiv.h            \
      ./widgets/vscale.h            \
      ./widgets/posedit.h           \
      ./widgets/genset.h            \
      ./widgets/shortcutcapturedialog.h         \
      ./widgets/wtscale.h           \
      ./widgets/mmath.h       \
      ./widgets/canvas.h            \
      ./widgets/shortcutconfig.h          \
      ./widgets/pitchlabel.h        \
      ./widgets/tools.h       \
      ./widgets/swidget.h           \
      ./widgets/splitter.h          \
      ./widgets/knob.h        \
      ./widgets/poslabel.h          \
      ./widgets/velocity.h          \
      ./widgets/midisyncimpl.h            \
      ./widgets/ttoolbar.h          \
      ./widgets/nentry.h            \
      ./widgets/action.h            \
      ./widgets/slider.h            \
      ./widgets/dentry.h            \
      ./widgets/siglabel.h          \
      ./widgets/checkbox.h          \
      ./widgets/mtscale.h           \
      ./widgets/spinboxFP.h         \
      ./widgets/view.h        \
      ./widgets/sigscale.h          \
      ./widgets/noteinfo.h          \
      ./widgets/doublelabel.h       \
      ./widgets/utils.h       \
      ./widgets/hitscale.h          \
      ./widgets/intlabel.h          \
      ./widgets/sigedit.h           \
      ./widgets/mlabel.h            \
      ./widgets/header.h            \
      ./widgets/tempolabel.h        \
      ./widgets/sliderbase.h        \
      ./widgets/scldraw.h           \
      ./widgets/citem.h       \
      ./widgets/songinfo.h       \
      ./mididev.h       \
      ./midictrl.h            \
      ./midiseq.h       \
      ./midifile.h            \
      ./shortcuts.h           \
      ./memory.h        \
      ./helper.h        \
      ./trackview.h        \
      ./plugin.h

SOURCES =  \
      ./ctrl/ctrlpanel.cpp          \
      ./ctrl/ctrledit.cpp           \
      ./ctrl/ctrlcanvas.cpp         \
      ./route.cpp       \
      ./undo.cpp        \
      ./midievent.cpp         \
      ./xml.cpp         \
      ./memory.cpp            \
      ./key.cpp         \
      ./midiseq.cpp           \
      ./song.cpp        \
      ./liste/listedit.cpp          \
      ./liste/editevent.cpp         \
      ./mixer/strip.cpp       \
      ./mixer/auxknob.cpp           \
      ./mixer/rack.cpp        \
      ./mixer/amixer.cpp            \
      ./mixer/routedialog.cpp       \
      ./mixer/panknob.cpp           \
      ./mixer/mstrip.cpp            \
      ./mixer/astrip.cpp            \
      ./mixer/meter.cpp       \
      ./transpose.cpp         \
      ./eventlist.cpp         \
      ./transport.cpp         \
      ./wavetrack.cpp         \
      ./audioprefetch.cpp           \
      ./helper.cpp            \
      ./miditransform.cpp           \
      ./ctrl.cpp        \
      ./sig.cpp         \
      ./confmport.cpp         \
      ./shortcuts.cpp         \
      ./audio.cpp       \
      ./part.cpp        \
      ./patchbay/patchbay.cpp       \
      ./appearance.cpp        \
      ./mpevent.cpp           \
      ./midi.cpp        \
      ./event.cpp       \
      ./midiedit/drumedit.cpp       \
      ./midiedit/piano.cpp          \
      ./midiedit/ecanvas.cpp        \
      ./midiedit/quantconfig.cpp          \
      ./midiedit/pianoroll.cpp            \
      ./midiedit/dlist.cpp          \
      ./midiedit/dcanvas.cpp        \
      ./midiedit/prcanvas.cpp       \
      ./midiedit/drummap.cpp        \
      ./plugin.cpp            \
      ./conf.cpp        \
      ./driver/jack.cpp       \
      ./driver/alsamidi.cpp         \
      ./driver/dummyaudio.cpp       \
      ./midictrl.cpp          \
      ./songfile.cpp          \
      ./gconfig.cpp           \
      ./mtc.cpp         \
      ./marker/marker.cpp           \
      ./marker/markerview.cpp       \
      ./master/master.cpp           \
      ./master/tscale.cpp           \
      ./master/lmaster.cpp          \
      ./master/masteredit.cpp       \
      ./sync.cpp        \
      ./wave.cpp        \
      ./midieditor.cpp        \
      ./waveedit/waveview.cpp       \
      ./waveedit/waveedit.cpp       \
      ./mplugins/mitplugin.cpp            \
      ./mplugins/midiitransform.cpp       \
      ./mplugins/mrconfig.cpp       \
      ./mplugins/mittranspose.cpp         \
      ./mplugins/midifilterimpl.cpp       \
      ./cobject.cpp           \
      ./midifile.cpp          \
      ./midiport.cpp          \
      ./exportmidi.cpp        \
      ./value.cpp       \
      ./synth.cpp       \
      ./waveevent.cpp         \
      ./icons.cpp       \
      ./instruments/minstrument.cpp       \
      ./instruments/editinstrument.cpp          \
      ./instruments/midictrledit.cpp            \
      ./app.cpp         \
      ./arranger/arranger.cpp       \
      ./arranger/trackinfo.cpp            \
      ./arranger/pcanvas.cpp        \
      ./arranger/alayout.cpp        \
      ./arranger/tlist.cpp          \
      ./audiotrack.cpp        \
      ./seqmsg.cpp            \
      ./widgets/swidget.cpp         \
      ./widgets/ttoolbutton.cpp           \
      ./widgets/siglabel.cpp        \
      ./widgets/pitchedit.cpp       \
      ./widgets/knob.cpp            \
      ./widgets/tempolabel.cpp            \
      ./widgets/dentry.cpp          \
      ./widgets/midisyncimpl.cpp          \
      ./widgets/oomwidgetsplug.cpp       \
      ./widgets/scldiv.cpp          \
      ./widgets/pitchlabel.cpp            \
      ./widgets/tools.cpp           \
      ./widgets/poslabel.cpp        \
      ./widgets/scldraw.cpp         \
      ./widgets/tb1.cpp       \
      ./widgets/gatetime.cpp        \
      ./widgets/sigscale.cpp        \
      ./widgets/vscale.cpp          \
      ./widgets/lcombo.cpp          \
      ./widgets/gcombo.cpp          \
      ./widgets/noteinfo.cpp        \
      ./widgets/mtscale.cpp         \
      ./widgets/scrollscale.cpp           \
      ./widgets/ttoolbar.cpp        \
      ./widgets/slider.cpp          \
      ./widgets/doublelabel.cpp           \
      ./widgets/citem.cpp           \
      ./widgets/intlabel.cpp        \
      ./widgets/sigedit.cpp         \
      ./widgets/combobox.cpp        \
      ./widgets/genset.cpp          \
      ./widgets/checkbox.cpp        \
      ./widgets/sliderbase.cpp            \
      ./widgets/comboQuant.cpp            \
      ./widgets/wtscale.cpp         \
      ./widgets/nentry.cpp          \
      ./widgets/canvas.cpp          \
      ./widgets/splitter.cpp        \
      ./widgets/posedit.cpp         \
      ./widgets/fontsel.cpp         \
      ./widgets/mixdowndialog.cpp         \
      ./widgets/ctrlcombo.cpp       \
      ./widgets/comment.cpp         \
      ./widgets/metronome.cpp       \
      ./widgets/mlabel.cpp          \
      ./widgets/spinboxFP.cpp       \
      ./widgets/drange.cpp          \
      ./widgets/velocity.cpp        \
      ./widgets/shortcutconfig.cpp        \
      ./widgets/header.cpp          \
      ./widgets/hitscale.cpp        \
      ./widgets/view.cpp            \
      ./widgets/bigtime.cpp         \
      ./widgets/filedialog.cpp            \
      ./widgets/sclif.cpp           \
      ./widgets/utils.cpp           \
      ./widgets/dimap.cpp           \
      ./widgets/mmath.cpp           \
      ./mididev.cpp           \
      ./node.cpp        \
      ./track.cpp       \
      ./pos.cpp         \
      ./globals.cpp           \
      ./importmidi.cpp        \
      ./thread.cpp            \
      ./help.cpp        \
      ./tempo.cpp

FORMS = \
      ./liste/editctrlbase.ui                    \
      ./mixer/midiportroutebase.ui               \
      ./mixer/routedialogbase.ui                 \
      ./patchbay/patchbaybase.ui                 \
      ./mplugins/mrconfigbase.ui                 \
      ./mplugins/midifilter.ui                   \
      ./mplugins/rhythmbase.ui                   \
      ./instruments/editinstrumentbase.ui        \
      ./instruments/ccontrolbase.ui              \
      ./widgets/synthconfigbase.ui               \
      ./widgets/mixdowndialogbase.ui             \
      ./widgets/aboutbox.ui                      \
      ./widgets/editnotedialogbase.ui            \
      ./widgets/commentbase.ui                   \
      ./widgets/fdialogbuttons.ui                \
      ./widgets/metronomebase.ui                 \
      ./widgets/gensetbase.ui                    \
      ./widgets/shortcutcapturedialogbase.ui     \
      ./widgets/mtrackinfobase.ui                \
      ./widgets/editmetadialogbase.ui            \
      ./widgets/gatetimebase.ui                  \
      ./widgets/configmidifilebase.ui            \
      ./widgets/editsysexdialogbase.ui           \
      ./widgets/wtrackinfobase.ui                \
      ./widgets/midisync.ui                      \
      ./widgets/appearancebase.ui                \
      ./widgets/velocitybase.ui                  \
      ./widgets/cliplisteditorbase.ui            \
      ./widgets/mittransposebase.ui              \
      ./widgets/transformbase.ui                 \
      ./widgets/transposebase.ui                 \
      ./widgets/editctrl7dialogbase.ui           \
      ./widgets/shortcutconfigbase.ui            \
      ./widgets/itransformbase.ui

TRANSLATIONS = \
      ../share/locale/oom_de.ts \
      ../share/locale/oom_fr.ts \
      ../share/locale/oom_sv_SE.ts \
      ../share/locale/oom_es.ts \
      ../share/locale/oom_ru.ts \
      ../share/locale/oom_pl.ts

