/*
 Copyright (C) 2010-2014 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRENCHBROOM_SPLITTER_H
#define TRENCHBROOM_SPLITTER_H

#include <QSplitter>

namespace TrenchBroom {
    namespace View {
        class SplitterHandle : public QSplitterHandle {
            Q_OBJECT
        public:
            using QSplitterHandle::QSplitterHandle;
        protected:
        public:
            QSize sizeHint() const override;

        protected:
            void paintEvent(QPaintEvent* event) override;
        };

        class Splitter : public QSplitter {
            Q_OBJECT
        public:
            using QSplitter::QSplitter;
        protected:
            QSplitterHandle* createHandle() override;
        };
    }
}


#endif //TRENCHBROOM_SPLITTER_H