/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#ifndef NIFTREEVIEW_H
#define NIFTREEVIEW_H

#include <QTreeView> // Inherited


//! Widget for showing a nif file as tree, list, or block details.
class NifTreeView final : public QTreeView
{
	Q_OBJECT

public:
	//! Constructor
	NifTreeView( QWidget * parent = 0, Qt::WindowFlags flags = 0 );
	//! Destructor
	~NifTreeView();

	//! Set the model used by the widget
	void setModel( QAbstractItemModel * model ) override final;
	//! Expand all branches
	void setAllExpanded( const QModelIndex & index, bool e );

	//! Accessor for EvalConditions
	bool evalConditions() const { return EvalConditions; }
	//! Is a row hidden?
	bool isRowHidden( int row, const QModelIndex & parent ) const;

	//! Minimum size
	QSize minimumSizeHint() const override final { return { 50, 50 }; }
	//! Default size
	QSize sizeHint() const override final { return { 400, 200 }; }

signals:
	//! Signal emmited when the current index changes; probably connected to NifSkope::select()
	void sigCurrentIndexChanged( const QModelIndex & );

public slots:
	//! Sets the root index
	void setRootIndex( const QModelIndex & index );
	//! Clear the root index; probably conncted to NifSkope::dList
	void clearRootIndex();

	//! Sets version evaluation conditions
	void setEvalConditions( bool );
	//! Sets real-time version condition evalutation (slow)
	void setRealTime( bool );

protected slots:
	//! Updates version conditions (connect to dataChanged)
	void updateConditions( const QModelIndex & topLeft, const QModelIndex & bottomRight );
	//! Recursively updates version conditions
	void updateConditionRecurse( const QModelIndex & index );
	//! Called when the current index changes
	void currentChanged( const QModelIndex & current, const QModelIndex & previous );

	//! Scroll to index; connected to expanded()
	void scrollExpand( const QModelIndex & index );

protected:
	void drawBranches( QPainter * painter, const QRect & rect, const QModelIndex & index ) const override final;
	void keyPressEvent( QKeyEvent * e ) override final;

	QStyleOptionViewItem viewOptions() const override final;

	bool EvalConditions;
	bool RealTimeEval;

	class BaseModel * nif;
};


#endif
